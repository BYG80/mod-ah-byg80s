#include "ScriptMgr.h"
#include "Config.h"
#include "AuctionHouseMgr.h"
#include "AuctionHouseSearcher.h"
#include "Item.h"
#include "ObjectMgr.h"
#include "World.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "Mail.h"
#include "GameTime.h"
#include "Player.h"
#include <vector>

struct BotAuctionItem {
    uint32 itemId;
    std::string category;
    uint32 chance;
};

class BotAuctionWorldScript : public WorldScript {
public:
    BotAuctionWorldScript() : WorldScript("BotAuctionWorldScript") {}

    void OnStartup() override {
        LOG_INFO("server.loading", "");
        LOG_INFO("server.loading", "╔════════════════════════════════════════════════════╗");
        LOG_INFO("server.loading", "║                 Byg80s AH Auction                  ║");
        LOG_INFO("server.loading", "╠════════════════════════════════════════════════════╣");
        LOG_INFO("server.loading", "║           Dynamic Auction House System             ║");
        LOG_INFO("server.loading", "║               for AzerothCore                      ║");
        LOG_INFO("server.loading", "║                                                    ║");
        LOG_INFO("server.loading", "║            • Automatic item auctions                ║");
        LOG_INFO("server.loading", "║            • Smart market pricing                   ║");
        LOG_INFO("server.loading", "║            • Automatic buy/sell AI                  ║");
        LOG_INFO("server.loading", "║            • Persistent economy system              ║");
        LOG_INFO("server.loading", "║════════════════════════════════════════════════════║");
        LOG_INFO("server.loading", "║                Created by Byg80s                   ║");
        LOG_INFO("server.loading", "║           Licensed under GNU AGPL v3.0             ║");
        LOG_INFO("server.loading", "╚════════════════════════════════════════════════════╝");
        LOG_INFO("server.loading", "");
        LoadItemsFromDB();
    }

    void OnUpdate(uint32 diff) override {
        static uint32 timer = 0;
        timer += diff;

        uint32 interval = sConfigMgr->GetOption<uint32>("BotAuction.UpdateInterval", 30000);
        if (timer < interval) return;
        timer = 0;

        if (!sConfigMgr->GetOption<bool>("BotAuction.Enable", true)) return;

        SellItems();
        BuyItems();
    }

private:
    std::vector<BotAuctionItem> _items;

    void LoadItemsFromDB() {
        _items.clear();
        QueryResult result = WorldDatabase.Query("SELECT item_id, category, chance FROM bot_auction_items");
        if (!result) return;
        do {
            Field* fields = result->Fetch();
            _items.push_back({ fields[0].Get<uint32>(), fields[1].Get<std::string>(), fields[2].Get<uint32>() });
        } while (result->NextRow());
    }

    uint32 CalculateSmartPrice(ItemTemplate const* proto, bool isBuyout) {
        if (!proto) return 1000;
        uint32 sellPrice = proto->SellPrice > 0 ? proto->SellPrice : 500;
        uint32 marketPrice = sellPrice * urand(8, 15);
        return isBuyout ? uint32(marketPrice * 1.4f) : marketPrice;
    }

    void SellItems() {
        uint32 maxGlobal = sConfigMgr->GetOption<uint32>("BotAuction.MaxBotAuctions", 1500);
        uint32 maxPerHouse = sConfigMgr->GetOption<uint32>("BotAuction.MaxPerHouse", 700);
        uint32 houses[] = { 2, 6, 7 };
        uint32 perCycle = sConfigMgr->GetOption<uint32>("BotAuction.ItemsPerCycle", 10);

        if (_items.empty()) return;

        for (uint32 houseId : houses) {
            AuctionHouseObject* ah = sAuctionMgr->GetAuctionsMapByHouseId(AuctionHouseId(houseId));
            if (!ah || ah->GetAuctions().size() >= maxPerHouse) continue;

            for (uint32 i = 0; i < perCycle; ++i) {
                BotAuctionItem& tItem = _items[urand(0, _items.size() - 1)];
                ItemTemplate const* proto = sObjectMgr->GetItemTemplate(tItem.itemId);
                if (!proto || !proto->SellPrice) continue;

                Item* item = Item::CreateItem(tItem.itemId, 1);
                if (!item) continue;

                CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
                item->SaveToDB(trans);

                AuctionEntry* auction = new AuctionEntry();
                auction->Id = sObjectMgr->GenerateAuctionID();
                auction->item_guid = item->GetGUID();
                auction->item_template = item->GetEntry();
                auction->itemCount = 1;
                auction->owner.Clear();
                auction->startbid = CalculateSmartPrice(proto, false);
                auction->buyout = CalculateSmartPrice(proto, true);
                auction->bid = 0;
                auction->bidder.Clear();
                auction->expire_time = uint32(GameTime::GetGameTime().count()) + 172800;
                auction->houseId = AuctionHouseId(houseId);
                auction->auctionHouseEntry = sAuctionHouseStore.LookupEntry(houseId);

                auction->SaveToDB(trans);
                CharacterDatabase.CommitTransaction(trans);
                sAuctionMgr->AddAItem(item);
                ah->AddAuction(auction);
            }
        }
    }

    void BuyItems() {
        uint32 houses[] = { 2, 6, 7 };
        uint32 now = uint32(GameTime::GetGameTime().count());

        bool forceBuy = sConfigMgr->GetOption<bool>("BotAuction.AlwaysBuyPlayerItems", false);
        uint32 chanceBuy = sConfigMgr->GetOption<uint32>("BotAuction.ChanceToBuy", 35);
        uint32 chanceBid = sConfigMgr->GetOption<uint32>("BotAuction.ChanceToBid", 50);
        uint32 minDelay = sConfigMgr->GetOption<uint32>("BotAuction.MinDelayMinutes", 10) * 60;
        uint32 maxBuys = sConfigMgr->GetOption<uint32>("BotAuction.MaxBuysPerCycle", 10);
        float maxMult = sConfigMgr->GetOption<float>("BotAuction.MaxPlayerPriceMultiplier", 2.0f);
        uint32 maxAbsPrice = sConfigMgr->GetOption<uint32>("BotAuction.MaxAbsolutePrice", 250000);
        ObjectGuid dummyBidder = ObjectGuid::Create<HighGuid::Player>(sConfigMgr->GetOption<uint32>("BotAuction.DummyBidderGUID", 1));

        for (uint32 houseId : houses) {
            AuctionHouseObject* ah = sAuctionMgr->GetAuctionsMapByHouseId(AuctionHouseId(houseId));
            if (!ah) continue;

            uint32 buysThisCycle = 0;
            std::vector<uint32> toDelete;

            for (auto const& [id, auction] : ah->GetAuctions()) {
                if (!auction || buysThisCycle >= maxBuys) continue;

                if (auction->expire_time > (now + 172800 - minDelay)) continue;

                if (!auction->owner.IsEmpty()) {
                    ItemTemplate const* proto = sObjectMgr->GetItemTemplate(auction->item_template);
                    if (!proto) continue;

                    uint32 maxAllowedPrice = (proto->SellPrice > 0) ? uint32(proto->SellPrice * maxMult) : maxAbsPrice;
                    if (auction->buyout > maxAllowedPrice || auction->buyout > maxAbsPrice) continue;

                    uint32 roll = urand(1, 100);

                    // LOGICA DE COMPRA (Prioriza forceBuy)
                    if ((forceBuy || roll <= chanceBuy) && auction->buyout > 0) {
                        CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
                        auction->bid = auction->buyout;
                        sAuctionMgr->SendAuctionSuccessfulMail(auction, trans);
                        auction->DeleteFromDB(trans);
                        CharacterDatabase.CommitTransaction(trans);
                        toDelete.push_back(auction->Id);
                        buysThisCycle++;
                    }
                    // LOGICA DE PUJA
                    else if (!forceBuy && roll <= (chanceBuy + chanceBid)) {
                        uint32 currentPrice = auction->bid > 0 ? auction->bid : auction->startbid;
                        uint32 raise = uint32(currentPrice * 0.05f) + 1000;
                        uint32 newBid = currentPrice + raise;

                        if (auction->buyout == 0 || newBid < auction->buyout) {
                            CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
                            if (!auction->bidder.IsEmpty())
                                sAuctionMgr->SendAuctionOutbiddedMail(auction, newBid, nullptr, trans);

                            auction->bidder = dummyBidder;
                            auction->bid = newBid;
                            sAuctionMgr->GetAuctionHouseSearcher()->UpdateBid(auction);
                            auction->SaveToDB(trans);
                            CharacterDatabase.CommitTransaction(trans);
                        }
                    }
                }
            }
            for (uint32 id : toDelete) {
                if (AuctionEntry* a = ah->GetAuction(id)) ah->RemoveAuction(a);
            }
        }
    }
};

void Addmod_bot_auctionScripts() { new BotAuctionWorldScript(); }
       

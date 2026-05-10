#include "ScriptMgr.h"
#include "Config.h"
#include "AuctionHouseMgr.h"
#include "Item.h"
#include "ObjectMgr.h"
#include "World.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "Mail.h"
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
        LOG_INFO("server.loading", "║        Dynamic Auction House System                ║");
        LOG_INFO("server.loading", "║              for AzerothCore                       ║");
        LOG_INFO("server.loading", "║                                                    ║");
        LOG_INFO("server.loading", "║           • Automatic item auctions                 ║");
        LOG_INFO("server.loading", "║           • Smart market pricing                    ║");
        LOG_INFO("server.loading", "║           • Automatic buy/sell AI                   ║");
        LOG_INFO("server.loading", "║           • Persistent economy system               ║");
        LOG_INFO("server.loading", "║════════════════════════════════════════════════════║");
        LOG_INFO("server.loading", "║                Created by Byg80s                   ║");
        LOG_INFO("server.loading", "║          Licensed under GNU AGPL v3.0              ║");
        LOG_INFO("server.loading", "╚════════════════════════════════════════════════════╝");
        LOG_INFO("server.loading", "");


        LoadItemsFromDB();
    }

    void OnUpdate(uint32 diff) override {
        static uint32 timer = 0;
        timer += diff;

        if (timer < sConfigMgr->GetOption<uint32>("BotAuction.UpdateInterval", 30000))
            return;

        timer = 0;

        if (!sConfigMgr->GetOption<bool>("BotAuction.Enable", true))
            return;

        SellItems();
        BuyItems();
    }

private:
    std::vector<BotAuctionItem> _items;

    void LoadItemsFromDB() {
        _items.clear();
        QueryResult result = WorldDatabase.Query("SELECT item_id, category, chance FROM bot_auction_items");
        if (!result) {
            LOG_ERROR("server.loading", "BotAuction: No se pudo leer la tabla bot_auction_items");
            return;
        }
        do {
            Field* fields = result->Fetch();
            _items.push_back({ fields[0].Get<uint32>(), fields[1].Get<std::string>(), fields[2].Get<uint32>() });
        } while (result->NextRow());
        LOG_INFO("server.loading", "BotAuction: {} items cargados correctamente.", _items.size());
    }

    void SellItems() {
        uint32 maxGlobal = sConfigMgr->GetOption<uint32>("BotAuction.MaxBotAuctions", 800);
        uint32 houses[] = { 2, 6, 7 };
        uint32 perCycle = sConfigMgr->GetOption<uint32>("BotAuction.ItemsPerCycle", 10);

        if (_items.empty()) return;

        CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
        for (uint32 i = 0; i < perCycle; ++i) {
            uint32 houseId = houses[urand(0, 2)];
            AuctionHouseObject* ah = sAuctionMgr->GetAuctionsMapByHouseId(AuctionHouseId(houseId));
            if (!ah) continue;

            // Seleccionamos un item al azar de tu lista de la DB
            BotAuctionItem& templateItem = _items[urand(0, _items.size() - 1)];

            Item* item = Item::CreateItem(templateItem.itemId, 1);
            if (!item) continue;

            item->SaveToDB(trans);
            AuctionEntry* auction = new AuctionEntry();
            auction->Id = sObjectMgr->GenerateAuctionID();
            auction->item_guid = item->GetGUID();
            auction->item_template = item->GetEntry();
            auction->itemCount = 1;
            auction->owner.Clear(); // Bot

            // Precios directos sin cálculos complejos para que funcione ya
            auction->startbid = 10000; // 1 oro
            auction->buyout = 15000;   // 1.5 oro
            auction->bidder.Clear();
            auction->expire_time = time(nullptr) + 86400;
            auction->houseId = AuctionHouseId(houseId);
            auction->auctionHouseEntry = sAuctionHouseStore.LookupEntry(houseId);

            auction->SaveToDB(trans);
            sAuctionMgr->AddAItem(item);
            ah->AddAuction(auction);
        }
        CharacterDatabase.CommitTransaction(trans);
    }

    void BuyItems() {
        uint32 houses[] = { 2, 6, 7 };
        for (uint32 houseId : houses) {
            AuctionHouseObject* ah = sAuctionMgr->GetAuctionsMapByHouseId(AuctionHouseId(houseId));
            if (!ah) continue;

            CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
            std::vector<uint32> toDelete;

            for (auto const& pair : ah->GetAuctions()) {
                AuctionEntry* auction = pair.second;
                if (!auction) continue;

                // SI NO ES UN BOT (Counter != 0), ES UN PLAYER -> COMPRAR
                if (auction->owner.GetCounter() != 0) {
                    auction->bidder = auction->owner;
                    auction->bid = auction->buyout;
                    sAuctionMgr->SendAuctionSuccessfulMail(auction, trans);
                    auction->DeleteFromDB(trans);
                    toDelete.push_back(auction->Id);
                }
            }

            for (uint32 id : toDelete) {
                if (AuctionEntry* a = ah->GetAuction(id)) ah->RemoveAuction(a);
            }
            CharacterDatabase.CommitTransaction(trans);
        }
    }
};

void Addmod_bot_auctionScripts() { new BotAuctionWorldScript(); }


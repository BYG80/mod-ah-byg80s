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
#include <unordered_map>

struct BotAuctionItem
{
    uint32 itemId;
    std::string category;
    uint32 chance;
};

class BotAuctionWorldScript : public WorldScript
{
public:
    BotAuctionWorldScript() : WorldScript("BotAuctionWorldScript") {}

    void OnStartup() override
    {
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
        LOG_INFO("server.loading", "BotAuction: Sistema iniciado y listo.");
    }

    void OnUpdate(uint32 diff) override
    {
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

    void LoadItemsFromDB()
    {
        _items.clear();
        QueryResult result = WorldDatabase.Query("SELECT item_id, category, chance FROM bot_auction_items");
        if (!result) return;

        do
        {
            Field* fields = result->Fetch();
            _items.push_back({ fields[0].Get<uint32>(), fields[1].Get<std::string>(), fields[2].Get<uint32>() });
        } while (result->NextRow());
    }

    uint32 GetBasePrice(uint32 itemId)
    {
        ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemId);
        if (!proto) return 10000;
        // Precio base basado en el valor de venta al vendedor o fallback
        uint32 base = proto->SellPrice > 0 ? proto->SellPrice * 5 : 1000;
        return uint32(float(base) * frand(0.8f, 1.2f));
    }

    void BuyItems()
    {
        uint32 maxBuys = sConfigMgr->GetOption<uint32>("BotAuction.MaxBuysPerCycle", 10);
        float maxPlayerMultiplier = sConfigMgr->GetOption<float>("BotAuction.MaxPlayerPriceMultiplier", 3.0f);
        uint32 playerBuyChance = sConfigMgr->GetOption<uint32>("BotAuction.PlayerBuyChance", 80);
        uint32 botBuyChance = sConfigMgr->GetOption<uint32>("BotAuction.BotBuyChance", 30);
        bool alwaysBuyPlayer = sConfigMgr->GetOption<bool>("BotAuction.AlwaysBuyPlayerItems", false);

        uint32 houses[] = { 2, 6, 7 };
        uint32 bought = 0;

        for (uint32 houseId : houses)
        {
            AuctionHouseObject* ah = sAuctionMgr->GetAuctionsMapByHouseId(AuctionHouseId(houseId));
            if (!ah) continue;

            CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
            std::vector<uint32> toDelete;

            for (auto const& pair : ah->GetAuctions())
            {
                if (bought >= maxBuys) break;

                AuctionEntry* auction = pair.second;
                if (!auction || auction->buyout == 0) continue;

                bool isBotOwner = auction->owner.IsEmpty();
                uint32 marketPrice = GetBasePrice(auction->item_template);

                if (!isBotOwner) // Lógica para comprar al JUGADOR
                {
                    if (!alwaysBuyPlayer && urand(0, 100) > playerBuyChance)
                        continue;

                    // FILTRO DE PRECIO JUSTO para jugadores
                    if (auction->buyout > (marketPrice * maxPlayerMultiplier))
                        continue;
                }
                else // Lógica para compra entre BOTS (Mercado interno)
                {
                    if (urand(0, 100) > botBuyChance)
                        continue;
                }

                // Ejecución de la compra
                auction->bidder = auction->owner;
                auction->bid = auction->buyout;

                sAuctionMgr->SendAuctionSuccessfulMail(auction, trans);
                auction->DeleteFromDB(trans);
                toDelete.push_back(auction->Id);
                bought++;
            }

            for (uint32 id : toDelete)
                if (AuctionEntry* a = ah->GetAuction(id)) ah->RemoveAuction(a);

            CharacterDatabase.CommitTransaction(trans);
        }
    }

    void SellItems()
    {
        uint32 maxGlobal = sConfigMgr->GetOption<uint32>("BotAuction.MaxBotAuctions", 800);
        if (CountTotalBots() >= maxGlobal) return;

        uint32 perCycle = sConfigMgr->GetOption<uint32>("BotAuction.ItemsPerCycle", 10);
        uint32 houses[] = { 2, 6, 7 };
        uint32 added = 0;

        CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
        while (added < perCycle)
        {
            uint32 houseId = houses[urand(0, 2)];
            AuctionHouseObject* ah = sAuctionMgr->GetAuctionsMapByHouseId(AuctionHouseId(houseId));
            if (!ah || CountBotAuctions(ah) >= 700) continue;

            uint32 itemId = GetRandomItem();
            if (!itemId || CountItem(ah, itemId) >= 4) continue;

            Item* item = Item::CreateItem(itemId, urand(1, 3));
            if (!item) continue;

            item->SetOwnerGUID(ObjectGuid::Empty);
            item->SaveToDB(trans);

            AuctionEntry* auction = new AuctionEntry();
            auction->Id = sObjectMgr->GenerateAuctionID();
            auction->owner.Clear();
            auction->item_guid = item->GetGUID();
            auction->item_template = itemId;
            auction->itemCount = item->GetCount();
            uint32 price = GetBasePrice(itemId);
            auction->startbid = price;
            auction->buyout = uint32(float(price) * frand(1.1f, 1.4f));
            auction->bidder = ObjectGuid::Empty;
            auction->expire_time = time(nullptr) + urand(14400, 86400);
            auction->houseId = AuctionHouseId(houseId);
            auction->auctionHouseEntry = sAuctionHouseStore.LookupEntry(houseId);

            auction->SaveToDB(trans);
            ah->AddAuction(auction);
            sAuctionMgr->AddAItem(item);
            added++;
        }
        CharacterDatabase.CommitTransaction(trans);
    }

    uint32 GetRandomItem()
    {
        if (_items.empty()) return 0;
        uint32 totalChance = 0;
        for (auto const& item : _items) totalChance += item.chance;
        uint32 roll = urand(1, totalChance), current = 0;
        for (auto const& item : _items) { current += item.chance; if (roll <= current) return item.itemId; }
        return _items.front().itemId;
    }

    uint32 CountBotAuctions(AuctionHouseObject* ah)
    {
        uint32 count = 0;
        for (auto const& pair : ah->GetAuctions()) if (pair.second && pair.second->owner.IsEmpty()) ++count;
        return count;
    }

    uint32 CountTotalBots()
    {
        uint32 total = 0, houses[] = { 2, 6, 7 };
        for (uint32 h : houses) if (auto* ah = sAuctionMgr->GetAuctionsMapByHouseId(AuctionHouseId(h))) total += CountBotAuctions(ah);
        return total;
    }

    uint32 CountItem(AuctionHouseObject* ah, uint32 itemId)
    {
        uint32 count = 0;
        for (auto const& pair : ah->GetAuctions()) if (pair.second && pair.second->item_template == itemId) ++count;
        return count;
    }
};

void Addmod_bot_auctionScripts() { new BotAuctionWorldScript(); }





























    
 

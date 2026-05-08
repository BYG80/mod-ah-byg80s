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

        LOG_INFO("server.loading",
            "BotAuction: Ready with {} items",
            _items.size());
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

        try
        {
            SellItems();
            BuyItems();
        }
        catch (std::exception const& e)
        {
            LOG_ERROR("server.loading",
                "BotAuction ERROR: {}", e.what());
        }
    }

private:

    std::vector<BotAuctionItem> _items;

    struct PriceData
    {
        uint32 price;
        time_t lastUpdate;
    };

    std::unordered_map<uint32, PriceData> _priceCache;

    /*==============================================*/
    /* LOAD ITEMS                                   */
    /*==============================================*/

    void LoadItemsFromDB()
    {
        _items.clear();

        QueryResult result =
            WorldDatabase.Query(
                "SELECT item_id, category, chance FROM bot_auction_items");

        if (!result)
        {
            LOG_ERROR("server.loading",
                "BotAuction: bot_auction_items empty");
            return;
        }

        do
        {
            Field* fields = result->Fetch();

            uint32 itemId = fields[0].Get<uint32>();

            if (!sObjectMgr->GetItemTemplate(itemId))
                continue;

            BotAuctionItem item;

            item.itemId = itemId;
            item.category = fields[1].Get<std::string>();
            item.chance = std::max<uint32>(1, fields[2].Get<uint32>());

            _items.push_back(item);

        } while (result->NextRow());

        LOG_INFO("server.loading",
            "BotAuction: Loaded {} items",
            _items.size());
    }

    /*==============================================*/
    /* RANDOM ITEM                                  */
    /*==============================================*/

    uint32 GetRandomItem()
    {
        if (_items.empty())
            return 0;

        uint32 totalChance = 0;

        for (auto const& item : _items)
            totalChance += item.chance;

        uint32 roll = urand(1, totalChance);

        uint32 current = 0;

        for (auto const& item : _items)
        {
            current += item.chance;

            if (roll <= current)
                return item.itemId;
        }

        return _items.front().itemId;
    }

    /*==============================================*/
    /* PRICE SYSTEM                                 */
    /*==============================================*/

    uint32 GetBasePrice(uint32 itemId)
    {
        time_t now = time(nullptr);

        auto itr = _priceCache.find(itemId);

        if (itr != _priceCache.end())
        {
            if ((now - itr->second.lastUpdate) < 600)
                return itr->second.price;
        }

        ItemTemplate const* proto =
            sObjectMgr->GetItemTemplate(itemId);

        if (!proto)
            return 10000;

        uint32 base = 1000;

        switch (proto->Quality)
        {
        case ITEM_QUALITY_POOR:
        case ITEM_QUALITY_NORMAL:
            base = urand(1000, 5000);
            break;

        case ITEM_QUALITY_UNCOMMON:
            base = urand(5000, 25000);
            break;

        case ITEM_QUALITY_RARE:
            base = urand(25000, 80000);
            break;

        case ITEM_QUALITY_EPIC:
            base = urand(80000, 250000);
            break;

        default:
            base = urand(1000, 5000);
            break;
        }

        uint32 finalPrice =
            uint32(float(base) * frand(0.9f, 1.2f));

        _priceCache[itemId] =
        {
            finalPrice,
            now
        };

        return finalPrice;
    }

    /*==============================================*/
    /* HELPERS                                      */
    /*==============================================*/

    uint32 CountBotAuctions(AuctionHouseObject* ah)
    {
        uint32 count = 0;

        for (auto const& pair : ah->GetAuctions())
        {
            AuctionEntry* auction = pair.second;

            if (!auction)
                continue;

            if (auction->owner.IsEmpty())
                ++count;
        }

        return count;
    }

    uint32 CountTotalBots()
    {
        uint32 houses[] = { 2, 6, 7 };

        uint32 total = 0;

        for (uint32 houseId : houses)
        {
            AuctionHouseObject* ah =
                sAuctionMgr->GetAuctionsMapByHouseId(
                    AuctionHouseId(houseId));

            if (!ah)
                continue;

            total += CountBotAuctions(ah);
        }

        return total;
    }

    uint32 CountItem(AuctionHouseObject* ah, uint32 itemId)
    {
        uint32 count = 0;

        for (auto const& pair : ah->GetAuctions())
        {
            AuctionEntry* auction = pair.second;

            if (!auction)
                continue;

            if (auction->item_template == itemId)
                ++count;
        }

        return count;
    }

    /*==============================================*/
    /* SELL ITEMS                                   */
    /*==============================================*/

    void SellItems()
    {
        uint32 maxGlobal =
            sConfigMgr->GetOption<uint32>(
                "BotAuction.MaxBotAuctions", 800);

        if (CountTotalBots() >= maxGlobal)
            return;

        uint32 perCycle =
            sConfigMgr->GetOption<uint32>(
                "BotAuction.ItemsPerCycle", 10);

        uint32 maxPerHouse =
            sConfigMgr->GetOption<uint32>(
                "BotAuction.MaxPerHouse", 700);

        uint32 houses[] = { 2, 6, 7 };

        uint32 added = 0;

        CharacterDatabaseTransaction trans =
            CharacterDatabase.BeginTransaction();

        while (added < perCycle)
        {
            uint32 houseId = houses[urand(0, 2)];

            AuctionHouseObject* ah =
                sAuctionMgr->GetAuctionsMapByHouseId(
                    AuctionHouseId(houseId));

            if (!ah)
                continue;

            if (CountBotAuctions(ah) >= maxPerHouse)
                continue;

            uint32 itemId = GetRandomItem();

            if (!itemId)
                continue;

            if (CountItem(ah, itemId) >= 4)
                continue;

            Item* item =
                Item::CreateItem(itemId, urand(1, 3));

            if (!item)
                continue;

            item->SetOwnerGUID(ObjectGuid::Empty);

            item->SaveToDB(trans);

            AuctionEntry* auction = new AuctionEntry();

            auction->Id =
                sObjectMgr->GenerateAuctionID();

            auction->owner.Clear();

            // TU CORE USA GUID COMPLETO
            auction->item_guid = item->GetGUID();

            auction->item_template = itemId;
            auction->itemCount = item->GetCount();

            uint32 price = GetBasePrice(itemId);

            auction->startbid = price;

            auction->buyout =
                uint32(float(price) * frand(1.1f, 1.4f));

            auction->bid = 0;
            auction->deposit = 0;

            auction->bidder = ObjectGuid::Empty;

            auction->expire_time =
                time(nullptr) + urand(14400, 86400);

            auction->auctionHouseEntry =
                sAuctionHouseStore.LookupEntry(houseId);

            auction->houseId =
                AuctionHouseId(houseId);

            auction->SaveToDB(trans);

            ah->AddAuction(auction);

            sAuctionMgr->AddAItem(item);

            LOG_INFO("server.loading",
                "BotAuction SELL: AH={} ITEM={} PRICE={}",
                houseId,
                itemId,
                auction->buyout);

            ++added;
        }

        CharacterDatabase.CommitTransaction(trans);

        if (added > 0)
        {
            LOG_INFO("server.loading",
                "BotAuction: Added {} auctions",
                added);
        }
    }

    /*==============================================*/
    /* BUY ITEMS                                    */
    /*==============================================*/

    void BuyItems()
    {
        uint32 maxBuys =
            sConfigMgr->GetOption<uint32>(
                "BotAuction.MaxBuysPerCycle", 10);

        uint32 houses[] = { 2, 6, 7 };

        uint32 bought = 0;

        for (uint32 houseId : houses)
        {
            AuctionHouseObject* ah =
                sAuctionMgr->GetAuctionsMapByHouseId(
                    AuctionHouseId(houseId));

            if (!ah)
                continue;

            CharacterDatabaseTransaction trans =
                CharacterDatabase.BeginTransaction();

            std::vector<uint32> auctionsToDelete;

            for (auto const& pair : ah->GetAuctions())
            {
                if (bought >= maxBuys)
                    break;

                AuctionEntry* auction = pair.second;

                if (!auction)
                    continue;

                if (auction->buyout == 0)
                    continue;

                bool isBot = (auction->owner.GetCounter() == 0);

                // NO comprar bots
                if (isBot)
                    continue;

                LOG_INFO("server.loading",
                    "BotAuction BUY PLAYER: ITEM={} PRICE={}",
                    auction->item_template,
                    auction->buyout);

                // PAGAR ORO AL PLAYER
                sAuctionMgr->SendAuctionSuccessfulMail(
                    auction,
                    trans);

                // ELIMINAR ITEM
                Item* item =
                    sAuctionMgr->GetAItem(
                        auction->item_guid);

                if (item)
                {
                    sAuctionMgr->RemoveAItem(
                        auction->item_guid,
                        true,
                        &trans);
                }

                auction->DeleteFromDB(trans);

                auctionsToDelete.push_back(
                    auction->Id);

                ++bought;
            }

            for (uint32 auctionId : auctionsToDelete)
            {
                AuctionEntry* auction =
                    ah->GetAuction(auctionId);

                if (auction)
                    ah->RemoveAuction(auction);
            }

            CharacterDatabase.CommitTransaction(trans);
        }

        if (bought > 0)
        {
            LOG_INFO("server.loading",
                "BotAuction: Bought {} player auctions",
                bought);
        }
    }
};

void Addmod_bot_auctionScripts()
{
    new BotAuctionWorldScript();
}



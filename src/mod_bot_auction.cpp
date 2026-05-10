#include "ScriptMgr.h"
#include "Config.h"
#include "AuctionHouseMgr.h"
#include "Item.h"
#include "ObjectMgr.h"
#include "World.h"
#include "DatabaseEnv.h"
#include "Log.h"
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
    BotAuctionWorldScript()
        : WorldScript("BotAuctionWorldScript"), _timer(0) {
    }

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
            "BotAuction: Loaded {} items",
            _items.size());
    }

    void OnUpdate(uint32 diff) override
    {
        _timer += diff;

        uint32 interval =
            sConfigMgr->GetOption<uint32>("BotAuction.UpdateInterval", 30000);

        if (_timer < interval)
            return;

        _timer = 0;

        if (!sConfigMgr->GetOption<bool>("BotAuction.Enable", true))
            return;

        SellItems();
        BuyItems();
    }

private:

    uint32 _timer;
    std::vector<BotAuctionItem> _items;

    struct PriceData
    {
        uint32 price;
        time_t lastUpdate;
    };

    std::unordered_map<uint32, PriceData> _priceCache;

    /*==============================*/
    /* LOAD ITEMS                  */
    /*==============================*/

    void LoadItemsFromDB()
    {
        _items.clear();

        QueryResult result =
            WorldDatabase.Query("SELECT item_id, category, chance FROM bot_auction_items");

        if (!result)
        {
            LOG_ERROR("server.loading", "BotAuction: Empty item table");
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
    }

    /*==============================*/
    /* RANDOM ITEM                 */
    /*==============================*/

    uint32 GetRandomItem()
    {
        if (_items.empty())
            return 0;

        uint32 total = 0;

        for (auto const& i : _items)
            total += i.chance;

        uint32 roll = urand(1, total);
        uint32 current = 0;

        for (auto const& i : _items)
        {
            current += i.chance;
            if (roll <= current)
                return i.itemId;
        }

        return _items.front().itemId;
    }

    /*==============================*/
    /* PRICE SYSTEM                */
    /*==============================*/

    uint32 GetBasePrice(uint32 itemId)
    {
        time_t now = time(nullptr);

        auto itr = _priceCache.find(itemId);
        if (itr != _priceCache.end())
            if (now - itr->second.lastUpdate < 600)
                return itr->second.price;

        ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemId);
        if (!proto)
            return 10000;

        uint32 base = 1000;

        switch (proto->Quality)
        {
        case ITEM_QUALITY_POOR:
        case ITEM_QUALITY_NORMAL:
            base = urand(1000, 5000); break;
        case ITEM_QUALITY_UNCOMMON:
            base = urand(5000, 25000); break;
        case ITEM_QUALITY_RARE:
            base = urand(25000, 80000); break;
        case ITEM_QUALITY_EPIC:
            base = urand(80000, 200000); break;
        default:
            base = urand(1000, 5000); break;
        }

        uint32 finalPrice = uint32(float(base) * frand(0.9f, 1.2f));

        _priceCache[itemId] = { finalPrice, now };

        return finalPrice;
    }

    /*==============================*/
    /* SELL SYSTEM                 */
    /*==============================*/

    void SellItems()
    {
        if (_items.empty())
            return;

        uint32 perCycle =
            sConfigMgr->GetOption<uint32>("BotAuction.ItemsPerCycle", 8);

        uint32 houses[] = { 2, 6, 7 };

        CharacterDatabaseTransaction trans =
            CharacterDatabase.BeginTransaction();

        uint32 added = 0;
        uint32 safety = 0;

        while (added < perCycle && safety < 50)
        {
            safety++;

            uint32 houseId = houses[urand(0, 2)];
            auto ah = sAuctionMgr->GetAuctionsMapByHouseId(AuctionHouseId(houseId));

            if (!ah)
                continue;

            uint32 itemId = GetRandomItem();
            if (!itemId)
                continue;

            Item* item = Item::CreateItem(itemId, urand(1, 3));
            if (!item)
                continue;

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

            auction->bid = 0;
            auction->bidder = ObjectGuid::Empty;

            auction->expire_time = time(nullptr) + urand(14400, 86400);
            auction->houseId = AuctionHouseId(houseId);
            auction->auctionHouseEntry = sAuctionHouseStore.LookupEntry(houseId);

            auction->SaveToDB(trans);
            ah->AddAuction(auction);

            added++;
        }

        CharacterDatabase.CommitTransaction(trans);
    }

    /*==============================*/
    /* BUY SYSTEM                  */
    /*==============================*/

    void BuyItems()
    {
        bool forceBuyAll =
            sConfigMgr->GetOption<bool>("BotAuction.ForceBuyAll", false);

        uint32 maxBuys =
            sConfigMgr->GetOption<uint32>("BotAuction.MaxBuysPerCycle", 8);

        uint32 maxSpend =
            sConfigMgr->GetOption<uint32>("BotAuction.MaxGoldSpendPerCycle", 500000);

        uint32 minValue =
            sConfigMgr->GetOption<uint32>("BotAuction.MinBuyValue", 1000);

        uint32 bought = 0;
        uint32 spent = 0;

        uint32 houses[] = { 2, 6, 7 };

        for (uint32 houseId : houses)
        {
            auto ah = sAuctionMgr->GetAuctionsMapByHouseId(AuctionHouseId(houseId));
            if (!ah)
                continue;

            CharacterDatabaseTransaction trans =
                CharacterDatabase.BeginTransaction();

            auto auctionsCopy = ah->GetAuctions();

            std::vector<uint32> toRemove;

            for (auto const& p : auctionsCopy)
            {
                if (bought >= maxBuys)
                    break;

                AuctionEntry* auction = p.second;
                if (!auction || auction->buyout == 0)
                    continue;

                if (auction->owner.IsEmpty())
                    continue;

                if (!forceBuyAll)
                {
                    uint32 chance =
                        sConfigMgr->GetOption<uint32>("BotAuction.PlayerBuyChance", 25);

                    if (urand(1, 100) > chance)
                        continue;
                }
                else
                {
                    if (auction->buyout < minValue)
                        continue;

                    if (spent + auction->buyout > maxSpend)
                        continue;

                    spent += auction->buyout;
                }

                sAuctionMgr->SendAuctionSuccessfulMail(auction, trans);

                auction->DeleteFromDB(trans);
                toRemove.push_back(auction->Id);

                bought++;
            }

            for (uint32 id : toRemove)
                ah->RemoveAuction(ah->GetAuction(id));

            CharacterDatabase.CommitTransaction(trans);
        }
    }
};

void Addmod_bot_auctionScripts()
{
    new BotAuctionWorldScript();
}

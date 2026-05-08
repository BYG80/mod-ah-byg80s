# Byg80s AH Auction

Dynamic AI-powered Auction House module for AzerothCore.

This module automatically simulates a living Auction House economy by creating, buying and managing auctions with intelligent behavior and configurable pricing systems.

Compatible with AzerothCore and Playerbots-based servers.

---

## Features

- Automatic auction creation
- Dynamic smart pricing system
- AI buy/sell behavior
- Configurable economy settings
- Multi Auction House support
- Lightweight and optimized
- Playerbots core compatible
- Fully configurable via `.conf`

---

## Requirements

- AzerothCore WotLK
- Playerbots compatible core recommended
- C++17 compatible compiler

---

## Installation

### 1. Clone the module

Place the module inside your server modules folder:

```bash
cd modules
git clone https://github.com/BYG80/mod-ah-byg80s.git
```

---

### 2. Re-run CMake

After adding the module, re-run CMake and rebuild your server.

---

### 3. Import SQL

Create the required table inside your `world` database:

```sql
CREATE TABLE IF NOT EXISTS `bot_auction_items` (
  `item_id` INT UNSIGNED NOT NULL,
  `category` VARCHAR(50) NOT NULL DEFAULT '',
  `chance` INT UNSIGNED NOT NULL DEFAULT 1,
  PRIMARY KEY (`item_id`)
);
```

---

### 4. Add auction items

Example:

```sql
INSERT INTO bot_auction_items (`item_id`, `category`, `chance`) VALUES
(4306, 'trade', 100),
(14047, 'cloth', 80),
(1725, 'weapon', 40);
```

---

### 5. Configuration

Copy the configuration file:

```text
mod_ah_bgy80s.conf.dist
```

to:

```text
mod_ah_bgy80s.conf
```

and edit the settings as desired.

---

## Example Configuration

```ini
BotAuction.Enable = 1
BotAuction.UpdateInterval = 60000
BotAuction.MaxBotAuctions = 2000
BotAuction.ItemsPerCycle = 20
BotAuction.MaxBuysPerCycle = 10
```

---

## Console Banner

The module includes a startup banner displayed when the server launches.

Example:


 ╔════════════════════════════════════════════════════╗");
 ║                 Byg80s AH Auction                  ║");
 ╠════════════════════════════════════════════════════╣");
 ║        Dynamic Auction House System                ║");
 ║              for AzerothCore                       ║");
 ║                                                    ║");
 ║           • Automatic item auctions                ║");
 ║           • Smart market pricing                   ║");
 ║           • Automatic buy/sell AI                  ║");
 ║           • Persistent economy system              ║");
 ║════════════════════════════════════════════════════║");
 ║                Created by Byg80s                   ║");
 ║          Licensed under GNU AGPL v3.0              ║");
 ╚════════════════════════════════════════════════════╝");


## Notes

- The module uses empty GUIDs for bot-owned auctions.
- Smart prices adapt automatically based on market activity.
- Recommended for progressive or solo servers.

---

## License

This project is licensed under the GNU AGPL v3.0 license.

Based on AzerothCore:
https://github.com/azerothcore/azerothcore-wotlk

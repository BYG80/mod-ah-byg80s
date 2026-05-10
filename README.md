# Byg80s AH Auction (V2.0 - Multiple Identities Update)

========== ENGLISH =============

Dynamic AI-powered Auction House module for AzerothCore Playerbots.
This module automatically simulates a living Auction House economy by creating, buying, and managing auctions with intelligent behavior, multiple identities, and configurable pricing systems.

Designed specifically for AzerothCore Playerbots-based servers.

--------------------------------------------------------
NEW FEATURES (V2.0)
--------------------------------------------------------
- **Multi-Identity System**: The bot now uses a list of real character GUIDs to bid or buy, making the AH look populated by real players.
- **Infinite Gold Logic**: Bots automatically recharge their gold if they run low, ensuring they never fail to buy a player's item.
- **Smart Bidding**: Improved AI that decides between placing a bid or doing a direct buyout based on configurable chances.

--------------------------------------------------------
FEATURES
--------------------------------------------------------
- Automatic auction creation.
- Dynamic smart pricing system.
- AI buy/sell behavior (Bid vs Buyout).
- Configurable economy settings.
- Multi Auction House support (Alliance, Horde, Neutral).
- Lightweight and optimized.
- Playerbots core compatible.
- Fully configurable via .conf.

--------------------------------------------------------
COMPATIBILITY
--------------------------------------------------------
SUPPORTED CORE:
- AzerothCore WotLK.
- AzerothCore Playerbots Mod.
- AzerothCore Playerbot fork/core variants.

IMPORTANT: This module requires a Playerbots-compatible core to work correctly.
Recommended core: https://github.com/mod-playerbots/mod-playerbots

--------------------------------------------------------
INSTALLATION
--------------------------------------------------------
1. **Clone the module**:
   Place the module inside your server modules folder:
   cd modules
   git clone https://github.com/BYG80/mod-ah-byg80s.git

2. **Re-run CMake**:
   After adding the module, re-run CMake and rebuild your server.

3. **Import SQL**:
   Create the required table inside your world database:
   
   CREATE TABLE IF NOT EXISTS `bot_auction_items` (
     `item_id` INT UNSIGNED NOT NULL,
     `category` VARCHAR(50) NOT NULL DEFAULT '',
     `chance` INT UNSIGNED NOT NULL DEFAULT 1,
     PRIMARY KEY (`item_id`)
   );

4. **Configuration**:
   Copy `mod_ah_bgy80s.conf.dist` to `mod_ah_bgy80s.conf` and add your character GUIDs to `BotAuction.DummyBidderList`.

--------------------------------------------------------
EXAMPLE CONFIGURATION
--------------------------------------------------------
BotAuction.Enable = 1
BotAuction.UpdateInterval = 30000
BotAuction.AlwaysBuyPlayerItems = 0
BotAuction.ChanceToBuy = 35
BotAuction.ChanceToBid = 60
BotAuction.DummyBidderList = 4001,4002,4003,4005,4007,4010

--------------------------------------------------------
LICENSE
--------------------------------------------------------
This project is licensed under the GNU AGPL v3.0 license.


=============== ESPAÑOL =================

Módulo de Casa de Subastas con IA dinámica para AzerothCore Playerbots.
Simula una economía viva mediante la creación, compra y gestión inteligente de subastas con identidades múltiples y precios dinámicos.

Diseñado específicamente para servidores basados en AzerothCore Playerbots.

--------------------------------------------------------
NOVEDADES (V2.0)
--------------------------------------------------------
- **Sistema de Identidades Múltiples**: El bot utiliza una lista de GUIDs de personajes reales para pujar o comprar.
- **Oro Infinito**: Los bots recargan su oro automáticamente si se quedan sin fondos, garantizando que siempre puedan comprar tus objetos.
- **Pujas Inteligentes**: IA mejorada que decide entre pujar o comprar directamente según probabilidades configurables.

--------------------------------------------------------
CARACTERÍSTICAS
--------------------------------------------------------
- Creación automática de subastas.
- Sistema inteligente de precios dinámicos.
- Comportamiento de compra/venta realista (Puja vs Compra directa).
- Soporte para múltiples Casas de Subastas (Alianza, Horda, Neutral).
- Ligero y optimizado.
- Totalmente configurable mediante .conf.

--------------------------------------------------------
COMPATIBILIDAD
--------------------------------------------------------
CORE COMPATIBLE:
- AzerothCore WotLK.
- AzerothCore Playerbots Mod.
- Variantes/forks compatibles con Playerbots.

--------------------------------------------------------
INSTALACIÓN
--------------------------------------------------------
1. **Clonar el módulo**:
   cd modules
   git clone https://github.com/BYG80/mod-ah-byg80s.git

2. **Ejecutar CMake**:
   Vuelve a ejecutar CMake y recompila el servidor.

3. **Importar SQL**:
   Crea la tabla necesaria dentro de tu base de datos world (ver código SQL en la sección en inglés).

4. **Configurar**:
   Copia `mod_ah_bgy80s.conf.dist` a `mod_ah_bgy80s.conf` y edita los valores. Añade los GUIDs de tus personajes en `BotAuction.DummyBidderList`.

--------------------------------------------------------
NOTAS
--------------------------------------------------------
- Las subastas puestas por el bot aparecen con GUID vacío para diferenciarlas.
- Los precios inteligentes se adaptan automáticamente según la actividad.
- Recomendado para servidores Single Player o Progresivos.
- Optimizado para entornos con muchos Playerbots.

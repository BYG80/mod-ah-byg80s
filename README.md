# Byg80s AH Auction

==========ENGLISH =============


Dynamic AI-powered Auction House module for AzerothCore Playerbots.

This module automatically simulates a living Auction House economy by creating, 
buying and managing auctions with intelligent behavior and configurable pricing systems.

Designed specifically for AzerothCore Playerbots-based servers.

--------------------------------------------------------
COMPATIBILITY
--------------------------------------------------------

SUPPORTED CORE

- AzerothCore WotLK
- AzerothCore Playerbots Mod
- AzerothCore Playerbot fork/core variants

IMPORTANT

This module uses Auction House systems available in Playerbots-compatible AzerothCore builds.

It may NOT compile or work correctly on older or clean AzerothCore versions without Playerbots support.

Recommended core:
https://github.com/mod-playerbots/mod-playerbots

--------------------------------------------------------
FEATURES
--------------------------------------------------------

- Automatic auction creation
- Dynamic smart pricing system
- AI buy/sell behavior
- Configurable economy settings
- Multi Auction House support
- Lightweight and optimized
- Playerbots core compatible
- Fully configurable via .conf

--------------------------------------------------------
REQUIREMENTS
--------------------------------------------------------

- AzerothCore WotLK
- Playerbots-compatible core REQUIRED
- C++17 compatible compiler

--------------------------------------------------------
INSTALLATION
--------------------------------------------------------

1. Clone the module

Place the module inside your server modules folder:

cd modules
git clone https://github.com/BYG80/mod-ah-byg80s.git

--------------------------------------------------------

2. Re-run CMake

After adding the module, re-run CMake and rebuild your server.

--------------------------------------------------------

3. Import SQL

Create the required table inside your world database:

CREATE TABLE IF NOT EXISTS `bot_auction_items` (
  `item_id` INT UNSIGNED NOT NULL,
  `category` VARCHAR(50) NOT NULL DEFAULT '',
  `chance` INT UNSIGNED NOT NULL DEFAULT 1,
  PRIMARY KEY (`item_id`)
);

--------------------------------------------------------

4. Add auction items

Example:

INSERT INTO bot_auction_items (`item_id`, `category`, `chance`) VALUES
(4306, 'trade', 100),
(14047, 'cloth', 80),
(1725, 'weapon', 40);

--------------------------------------------------------

5. Configuration

Copy:

mod_ah_bgy80s.conf.dist

to:

mod_ah_bgy80s.conf

and edit the settings as desired.

--------------------------------------------------------
EXAMPLE CONFIGURATION
--------------------------------------------------------

BotAuction.Enable = 1
BotAuction.UpdateInterval = 60000
BotAuction.MaxBotAuctions = 2000
BotAuction.ItemsPerCycle = 20
BotAuction.MaxBuysPerCycle = 10

--------------------------------------------------------
CONSOLE BANNER
--------------------------------------------------------

╔══════════════════════════════════════════════╗
║              Byg80s AH Auction              ║
╠══════════════════════════════════════════════╣
║   Dynamic Auction House AI for AzerothCore  ║
║                                              ║
║   • Automatic auctions                       ║
║   • Smart pricing system                     ║
║   • Auto buy/sell behavior                   ║
║                                              ║
║   github.com/BYG80/mod-ah-byg80s             ║
╚══════════════════════════════════════════════╝

--------------------------------------------------------
NOTES
--------------------------------------------------------

- Bot-owned auctions use empty GUIDs.
- Smart prices adapt automatically based on market activity.
- Recommended for solo/progressive servers.
- Optimized for Playerbots environments.

--------------------------------------------------------
LICENSE
--------------------------------------------------------

This project is licensed under the GNU AGPL v3.0 license.

Based on AzerothCore:
https://github.com/azerothcore/azerothcore-wotlk

========================================================
======================= ESPAÑOL ========================
========================================================

Módulo de Casa de Subastas con IA dinámica para AzerothCore Playerbots.

Este módulo simula automáticamente una economía viva dentro de la Casa de Subastas mediante la creación, compra y gestión inteligente de subastas con un sistema configurable de precios dinámicos.

Diseñado específicamente para servidores basados en AzerothCore Playerbots.

--------------------------------------------------------
COMPATIBILIDAD
--------------------------------------------------------

CORE COMPATIBLE

- AzerothCore WotLK
- AzerothCore Playerbots Mod
- Variantes/forks compatibles con Playerbots

IMPORTANTE

Este módulo utiliza sistemas de Casa de Subastas disponibles en versiones compatibles con Playerbots.

Puede NO compilar o funcionar correctamente en versiones antiguas o limpias de AzerothCore sin soporte Playerbots.

Core recomendado:
https://github.com/mod-playerbots/mod-playerbots

--------------------------------------------------------
CARACTERÍSTICAS
--------------------------------------------------------

- Creación automática de subastas
- Sistema inteligente de precios dinámicos
- Comportamiento IA de compra/venta
- Economía configurable
- Soporte para múltiples Casas de Subastas
- Ligero y optimizado
- Compatible con cores Playerbots
- Totalmente configurable mediante .conf

--------------------------------------------------------
REQUISITOS
--------------------------------------------------------

- AzerothCore WotLK
- Core compatible con Playerbots OBLIGATORIO
- Compilador compatible con C++17

--------------------------------------------------------
INSTALACIÓN
--------------------------------------------------------

1. Clonar el módulo

Coloca el módulo dentro de la carpeta modules del servidor:

cd modules
git clone https://github.com/BYG80/mod-ah-byg80s.git

--------------------------------------------------------

2. Ejecutar nuevamente CMake

Después de añadir el módulo, vuelve a ejecutar CMake y recompila el servidor.

--------------------------------------------------------

3. Importar SQL

Crea la tabla necesaria dentro de tu base de datos world:

CREATE TABLE IF NOT EXISTS `bot_auction_items` (
  `item_id` INT UNSIGNED NOT NULL,
  `category` VARCHAR(50) NOT NULL DEFAULT '',
  `chance` INT UNSIGNED NOT NULL DEFAULT 1,
  PRIMARY KEY (`item_id`)
);

--------------------------------------------------------

4. Añadir objetos para las subastas

Ejemplo:

INSERT INTO bot_auction_items (`item_id`, `category`, `chance`) VALUES
(4306, 'trade', 100),
(14047, 'cloth', 80),
(1725, 'weapon', 40);

--------------------------------------------------------

5. Configuración

Copia:

mod_ah_bgy80s.conf.dist

a:

mod_ah_bgy80s.conf

y edita los valores según tus necesidades.

--------------------------------------------------------
CONFIGURACIÓN DE EJEMPLO
--------------------------------------------------------

BotAuction.Enable = 1
BotAuction.UpdateInterval = 60000
BotAuction.MaxBotAuctions = 2000
BotAuction.ItemsPerCycle = 20
BotAuction.MaxBuysPerCycle = 10

--------------------------------------------------------
BANNER DE CONSOLA
--------------------------------------------------------

╔══════════════════════════════════════════════╗
║              Byg80s AH Auction              ║
╠══════════════════════════════════════════════╣
║   Dynamic Auction House AI for AzerothCore  ║
║                                              ║
║   • Automatic auctions                       ║
║   • Smart pricing system                     ║
║   • Auto buy/sell behavior                   ║
║                                              ║
║   github.com/BYG80/mod-ah-byg80s             ║
╚══════════════════════════════════════════════╝

--------------------------------------------------------
NOTAS
--------------------------------------------------------

- Las subastas del bot utilizan GUID vacío.
- Los precios inteligentes se adaptan automáticamente según el mercado.
- Recomendado para servidores single player o progresivos.
- Optimizado para entornos Playerbots.

--------------------------------------------------------
LICENCIA
--------------------------------------------------------

Este proyecto está licenciado bajo GNU AGPL v3.0.

Basado en AzerothCore:
https://github.com/azerothcore/azerothcore-wotlk

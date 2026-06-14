# Belote / Coinche en ligne

Jeu de cartes multijoueur en **C++20** : client graphique **Raylib**, serveur **TCP Asio**
faisant autorité. Un seul moteur, partagé entre les jeux, conçu pour aller de la
**Belote** à la **Coinche** (et plus tard au **Tarot**) **sans duplication**.

> État : Belote et Coinche jouables de bout en bout (enchères + jeu de la carte +
> scoring), en réseau, avec des bots pour compléter la table.

---

## Sommaire
- [Fonctionnalités](#fonctionnalités)
- [Architecture](#architecture)
- [Compilation](#compilation)
- [Lancer une partie](#lancer-une-partie)
- [Jouer avec des amis](#jouer-avec-des-amis)
- [Tests](#tests)
- [Organisation du dépôt](#organisation-du-dépôt)
- [Limites connues & pistes](#limites-connues--pistes)

---

## Fonctionnalités

- **Deux modes au choix au lancement** (sélecteur dans le salon) :
  - **Belote** — enchère « à la retourne » (5 cartes + retourne, deux tours
    prendre/passer, redistribution si tout le monde passe) ; score avec **chute**
    (l'équipe preneuse doit battre la défense, sinon 0 / 162).
  - **Coinche** — enchère **chiffrée** (80 → 160, **capot**), avec **contre (×2)**
    et **surcontre (×4)** ; contrat réalisé ou *chute*.
- **Client/serveur faisant autorité** : le serveur valide chaque coup, le client
  n'affiche que ce que le serveur lui envoie (et recalcule les coups légaux côté
  client uniquement pour le confort visuel).
- **Jusqu'à 4 joueurs humains** ; les sièges vides sont tenus par des **bots**. Si
  un joueur se déconnecte en cours de partie, un bot reprend son siège.
- **Interface immédiate** dessinée à la main (cartes procédurales, aucune image).
- **Mode démo auto-piloté** (`--auto`) pour voir une partie se jouer seule.

## Architecture

Le cœur du projet est l'idée qu'une carte ne porte **que son identité** (couleur +
rang) : ses points et sa force dans un pli sont **calculés par les règles**, jamais
stockés. Tout le reste (deck, plis, joueurs, équipes, réseau) est partagé tel quel.

Ajouter un jeu = fournir **un triplet** injecté dans le moteur générique :

| Axe              | Belote                       | Coinche                          |
|------------------|------------------------------|----------------------------------|
| Jeu de la carte  | `BeloteRules`                | `BeloteRules` *(identique)*      |
| Enchère          | `BeloteBidding` (retourne)   | `CoincheBidding` (chiffrée)      |
| Scoring          | `scoreBeloteContract` (chute)| `scoreCoincheContract` (contrat) |
| Donne initiale   | 5 + retourne                 | 8, sans retourne                 |

Points clés :

- **`IGameRules`** (`common/.../engine/IGameRules.hpp`) — règles du jeu de la
  carte (deck, légalité « fournir / couper / monter », force, points, vainqueur du pli).
- **`IBiddingRules`** (`common/.../engine/IBiddingRules.hpp`) — l'enchère ; les
  règles possèdent **toutes** les transitions (à qui le tour, changements de tour,
  clôture). Types associés dans `engine/Bidding.hpp` (`Contract`, `BidAction`,
  `AuctionState`).
- **`GameTable`** — le cœur événementiel faisant autorité, avancé coup par coup
  (`startRound` → phase `Bidding` → `applyBid` → phase `Playing` → `applyMove`).
  Scoreur et règles d'enchère sont **injectés** ; `games/GameMode.hpp` assemble le
  bon `GameTable` selon le mode via `makeTable(GameMode, target)`.
- **`protocol/`** — encodage binaire big-endian, borné, avec *framing*
  `[u32 len][u16 type][payload]` ; messages `Login`, `StartGame{mode}`, `PlayCard`,
  `Bid{kind,suit,value}`, `GameState`, `RoundResult`, `MatchResult`, etc.
- **`server/`** — un seul `io_context` (donc aucun verrou) ; `GameServer` détient
  une table, place les humains, complète avec des bots, fait enchérir/jouer les bots.
- **`client/`** — boucle de rendu mono-thread qui pompe Asio sans bloquer ; scènes
  immédiates `MainMenu` → `Lobby` (choix Belote/Coinche) → `Game` (enchère puis table).

## Compilation

### Dépendances
- **CMake ≥ 3.20** et un compilateur **C++20** (GCC 11+, Clang 14+, MSVC 2022).
- Asio et Raylib sont récupérés automatiquement via **FetchContent** (rien à
  installer à la main).
- Le **client** (Raylib) nécessite, sous Linux, les en-têtes X11/OpenGL :
  ```bash
  sudo apt install libxinerama-dev libxcursor-dev libxi-dev \
                   libxrandr-dev libgl1-mesa-dev
  ```

### Construire

```bash
# Configurer (serveur + client activés ; ils sont OFF par défaut)
cmake -S . -B build -DCARDGAMES_BUILD_SERVER=ON -DCARDGAMES_BUILD_CLIENT=ON

# Compiler
cmake --build build
```

Options CMake :

| Option                   | Défaut | Rôle                                  |
|--------------------------|:------:|---------------------------------------|
| `CARDGAMES_BUILD_TESTS`  | `ON`   | tests unitaires / simulation          |
| `CARDGAMES_BUILD_SERVER` | `OFF`  | construit `belote_server`             |
| `CARDGAMES_BUILD_CLIENT` | `OFF`  | construit `belote_client` (Raylib)    |

> `common/` et `protocol/` sont du **C++ pur sans dépendance** et se compilent
> toujours — pratique pour les tests.

## Lancer une partie

**1. Démarrer le serveur** (port et objectif de points optionnels) :
```bash
./build/server/belote_server          # défaut : port 5555, objectif 1000
./build/server/belote_server 5555 500 # port 5555, partie à 500 points
```

**2. Lancer un client par joueur** :
```bash
./build/client/belote_client
```
Au menu, saisis un pseudo et l'adresse du serveur (défaut `127.0.0.1:5555`), puis
**Se connecter**. Dans le salon, choisis **Belote** ou **Coinche** et clique
**Démarrer** quand tout le monde est assis (les sièges libres deviennent des bots).

### Démo auto-pilotée
```bash
./build/client/belote_client --auto            # partie Belote jouée toute seule
./build/client/belote_client --auto --coinche  # idem en Coinche
```

## Jouer avec des amis

Une personne héberge le **serveur**, tout le monde lance le **client** vers son adresse.

- **Même réseau local** : l'hôte lance le serveur, récupère son IP locale
  (`ip a` → ex. `192.168.1.42`), les amis mettent cette IP + le port dans le menu.
- **Par Internet** : le plus simple est un **VPN overlay** (Tailscale, ZeroTier) —
  chacun obtient une IP privée stable, on se connecte comme en LAN, sans ouvrir de
  port. Alternatives : redirection du port `5555/TCP` sur la box (IP publique de
  l'hôte), ou héberger le serveur sur un petit **VPS**.

> Pour l'instant les binaires ne sont fournis que construits localement : chaque
> ami compile le client sur sa machine (le code est portable Windows/macOS/Linux
> mais seul Linux est testé à ce jour).

## Tests

```bash
ctest --test-dir build --output-on-failure
```

| Test             | Couvre                                                          |
|------------------|----------------------------------------------------------------|
| `common_smoke`   | briques de base (cartes, deck…)                                |
| `belote_sim`     | 5000 donnes aléatoires : légalité + conservation des points    |
| `gametable_test` | une partie complète pilotée comme le serveur                   |
| `bidding_test`   | enchère Belote « à la retourne » (tours, prise, donne nulle)   |
| `coinche_test`   | enchère chiffrée (paliers, contre/surcontre) + match complet   |
| `protocol_test`  | sérialisation / *framing* des messages                         |

## Organisation du dépôt

```
common/      moteur générique + jeux (C++ pur, sans dépendance)
  cards/       Card, Deck, Suit
  engine/      GameTable, IGameRules, IBiddingRules, Bidding, Trick, Player…
  games/belote/   BeloteRules, BeloteBidding, BeloteScoring, BeloteTable
  games/coinche/  CoincheBidding, CoincheScoring, CoincheTable
  games/GameMode.hpp   sélection du mode (makeTable / botBid)
protocol/    encodage binaire + messages réseau
server/      GameServer (Asio) + belote_server ; tools/ (client headless)
client/      belote_client (Raylib) : scènes, rendu des cartes, réseau
```

## Limites connues & pistes

- **Une seule table**, pas de salons multiples ni de code d'invitation.
- **Pas de reconnexion** : un siège quitté est repris par un bot (pas de reprise).
- Pas de comptes, pas de chiffrement applicatif, pseudos **ASCII** uniquement.
- Le **barème Coinche** est une variante volontairement simplifiée (ajustable).
- Pas encore de builds **Windows/macOS** packagés.

Pistes naturelles : barème Coinche complet (annonces), salons + codes d'invitation,
reconnexion, builds multi-plateformes, et à terme le **Tarot** (le moteur est prêt
pour : `Suit::Trump`/`None` existent déjà).

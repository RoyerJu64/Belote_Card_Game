
# Projet : Jeu de Tarot / Belote / Coinche en ligne

## Objectif

Développer une application desktop multijoueur en ligne avec :

- Client graphique en C++20 + Raylib
- Serveur dédié en C++20 + Asio
- Architecture client-serveur autoritaire
- Support progressif : Belote → Coinche → Tarot

---

# 1. Architecture générale

```text
+----------------+
|  Client Raylib |
+----------------+
        |
        | TCP
        |
        v
+----------------+
| Serveur dédié  |
+----------------+
        |
        v
+----------------+
| Etat du jeu    |
+----------------+
```

Le serveur est l'unique source de vérité :
- Distribution des cartes
- Validation des coups
- Gestion des scores
- Synchronisation des clients

---

# 2. Technologies

## Client
- C++20
- Raylib
- CMake

## Serveur
- C++20
- Asio (standalone)
- CMake

## Outils recommandés
- Git
- GitHub
- Visual Studio ou CLion
- Valgrind / Address Sanitizer

---

# 3. Arborescence du projet

```text
TarotOnline/

├── client/
│   ├── src/
│   ├── include/
│   ├── assets/
│   └── CMakeLists.txt
│
├── server/
│   ├── src/
│   ├── include/
│   └── CMakeLists.txt
│
├── common/
│   ├── src/
│   ├── include/
│   └── CMakeLists.txt
│
└── CMakeLists.txt
```

---

# 4. Phase 1 : Moteur de jeu

## 4.1 Cartes

Créer :
- Couleur
- Valeur
- Carte

## 4.2 Paquet

Fonctionnalités :
- génération
- mélange
- pioche

## 4.3 Joueur

Contient :
- nom
- main de cartes
- score

## 4.4 Pli

Contient :
- cartes jouées
- gagnant

## 4.5 Partie Belote

Fonctions :
- distribuer()
- jouerCarte()
- calculerPli()
- calculerScore()

## Validation

Lancer des milliers de parties simulées.

---

# 5. Phase 2 : Protocole réseau

## Définition des messages

```cpp
enum class MessageType
{
    Login,
    JoinLobby,
    LeaveLobby,
    StartGame,
    PlayCard,
    GameState,
    Chat
};
```

## Packet

```cpp
struct Packet
{
    MessageType type;
    std::vector<uint8_t> data;
};
```

## Sérialisation

Créer :
- Serializer
- Deserializer

Support :
- Carte
- Joueur
- Partie
- Packet

---

# 6. Phase 3 : Serveur TCP

## Pourquoi TCP ?

- Peu de bande passante
- Besoin de fiabilité
- Jeu au tour par tour

UDP n'est pas nécessaire.

## Classes

### Server

Responsabilités :
- écouter
- accepter les connexions
- distribuer les messages

### ClientSession

Responsabilités :
- gérer un joueur connecté
- envoyer / recevoir

---

# 7. Phase 4 : Lobby

Créer :

```cpp
class Lobby
{
};
```

Fonctionnalités :
- création de partie
- rejoindre une partie
- quitter une partie
- code d'invitation

Exemple :

```text
A9K4Q
```

---

# 8. Phase 5 : Etat partagé du jeu

Créer :

```cpp
class GameState
{
};
```

Contient :
- tour courant
- cartes jouées
- score équipe 1
- score équipe 2
- historique

Le serveur possède toujours l'état officiel.

---

# 9. Phase 6 : Client Raylib

Créer plusieurs scènes :

- MainMenuScene
- LobbyScene
- GameScene
- ResultScene

Créer un gestionnaire de scènes.

---

# 10. Phase 7 : Affichage des cartes

Préparer :
- 32 cartes belote
- 78 cartes tarot

Créer :

```cpp
class CardRenderer
{
};
```

Fonctions :
- drawHand()
- drawTable()
- drawTrump()

---

# 11. Phase 8 : Interaction utilisateur

Flux :

```text
Joueur clique
      ↓
Client envoie PlayCard
      ↓
Serveur valide
      ↓
Mise à jour GameState
      ↓
Broadcast aux clients
```

---

# 12. Phase 9 : Synchronisation

Principe fondamental :

Le client n'a jamais le droit de modifier l'état officiel.

Toujours :

```text
Client
 ↓
Serveur
 ↓
Clients
```

---

# 13. Phase 10 : Chat

Message :

```cpp
ChatMessage
```

Fonctionnalités :
- chat de salon
- chat de partie

---

# 14. Phase 11 : Coinche

Ajouter :
- annonces
- coinche
- surcoinche

Créer :

```cpp
class CoincheGame : public BeloteGame
{
};
```

---

# 15. Phase 12 : Tarot

Ajouter :
- atouts
- excuse
- chien
- poignées
- petit au bout

Créer :

```cpp
class TarotGame
{
};
```

Réutiliser toute l'infrastructure réseau existante.

---

# 16. Phase 13 : Persistance

Sauvegarder :

```json
{
    "nom":"Julien",
    "parties":45,
    "victoires":27
}
```

Fonctionnalités :
- statistiques
- historique
- classement

---

# 17. Phase 14 : Déploiement

## Serveur

VPS :
- OVH
- Hetzner
- Scaleway

Compilation :

```bash
cmake ..
make -j
```

Lancement :

```bash
./TarotServer
```

## Client

Distribution :
- Windows
- Linux

---

# 18. Fonctionnalités avancées

## IA

Créer :

```cpp
class BotPlayer
{
};
```

## Reconnexion

Permettre :
- reprise de partie
- reconnexion automatique

## Spectateurs

Mode observateur.

## Classement ELO

Système de progression.

---

# 19. Bonnes pratiques

- C++20 moderne
- RAII
- smart pointers
- const-correctness
- séparation .hpp/.cpp
- tests unitaires
- architecture modulaire
- documentation continue

---

# 20. Prompt de développement

> Je développe un jeu de cartes multijoueur en ligne en C++20 avec Raylib pour le client graphique et Asio pour le serveur TCP. L'architecture est client-serveur autoritaire. Génère un code professionnel, modulaire, moderne C++20, utilisant RAII, smart pointers, const-correctness et séparation .hpp/.cpp. Fournis l'arborescence des fichiers, les diagrammes UML simplifiés et les explications d'architecture avant chaque implémentation. Le code doit être évolutif pour supporter successivement Belote, Coinche et Tarot sans duplication. Priorité à la robustesse réseau, à la maintenabilité et aux bonnes pratiques de génie logiciel.

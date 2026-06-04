# 🛠️ Minimake - Réimplémentation de make

![C](https://img.shields.io/badge/Language-C-blue.svg)
![Shell](https://img.shields.io/badge/Testing-Shell-green.svg)
![EPITA](https://img.shields.io/badge/School-EPITA-red.svg)

Il s'agit du développement d'une version simplifiée du célèbre outil de compilation Unix `make`.

## 📝 Description

L'objectif principal de ce projet est de recréer la logique de résolution de dépendances et d'automatisation de la chaîne de compilation. Le code a été rédigé en respectant la norme C99 sous des contraintes strictes, avec l'interdiction d'utiliser des fonctions de facilité comme `system()`, `popen()`, ou la bibliothèque `regex.h`.

Les fonctionnalités et contraintes principales de notre implémentation sont :

- **Parsing et Exécution** : Analyse personnalisée des règles, des dépendances et des recettes. L'exécution des commandes shell se fait de manière sécurisée via des processus séparés (`/bin/sh -c`).
- **Gestion des cibles** : Résolution dynamique des cibles et vérification des dates de modification (*timestamps*) pour éviter de recompiler les fichiers déjà à jour (*up-to-date*).
- **Gestion des variables** : Support de l'expansion récursive des variables, de l'environnement, ainsi que des variables spéciales telles que `$@`, `$<`, et `$^`.
- **Fonctionnalités avancées** : Prise en charge des cibles virtuelles (`.PHONY`), des règles génériques (*Pattern rules* comme `%.o: %.c`) résolues en fonction de la taille de la racine (*stem*), et de l'option `-f` pour spécifier un ou plusieurs fichiers d'entrée.
- **Self-building (Bonus)** : L'outil est capable de se compiler lui-même de manière autonome à l'aide de son propre fichier `Minimakefile`.

## 🏗️ Architecture du Projet

Le code source est modulaire et séparé pour isoler la logique du programme de ses tests :

- 📂 `src/` : Contient le code source principal de l'application (analyseur syntaxique, gestion de l'arbre de dépendances, exécuteur).
- 📂 `tests/` : Contient la suite de tests fonctionnels automatisés via des scripts shell pour valider le comportement face à divers Makefiles.
- 📄 `Makefile` : Le fichier d'orchestration situé à la racine pour automatiser la première compilation du projet.
- 📄 `Minimakefile` : Le fichier de build compatible avec notre implémentation, permettant au projet de s'auto-compiler.

## ⚙️ Prérequis

Pour compiler et tester ce projet, votre environnement (idéalement Linux / UNIX) doit disposer des outils suivants :

- Compilateur **GCC** ou **Clang**
- **Make** (pour la première compilation)
- Un interpréteur **Shell** (`sh`/`bash`) pour lancer les tests.

Installation des dépendances (Ubuntu/Debian) :

```bash
sudo apt update
sudo apt install build-essential
```

## 🚀 Compilation & Exécution (Makefile)

Le projet utilise un `Makefile` principal situé à la racine pour faciliter l'intégration et la compilation.

### 1. Compilation simple

Pour générer l'exécutable `minimake`, exécutez la commande suivante à la racine du projet :

```bash
make
```

### 2. Lancement de la suite de tests

Pour exécuter automatiquement la suite de tests fonctionnels :

```bash
make check
```

> **Note** : Ce projet est testé rigoureusement sur divers scénarios (erreurs de syntaxe, graphes cycliques, variables imbriquées) pour garantir un comportement au plus proche du `make` GNU standard.

### 3. Nettoyage du projet

Pour supprimer tous les fichiers produits lors de la compilation (`.o`, exécutables) afin de repartir sur un répertoire propre :

```bash
make clean
```

## 🖱️ Notice d'utilisation

Une fois compilé, vous pouvez utiliser `minimake` dans vos projets pour remplacer l'outil standard `make`.

### 1. Exécution classique

Par défaut, l'outil cherche un fichier nommé `Makefile` ou `makefile` dans le répertoire courant et exécute la première règle.

```bash
./minimake
```

### 2. Spécifier une cible

Vous pouvez construire une cible spécifique (ou plusieurs) définie dans votre fichier.

```bash
./minimake clean
```

### 3. Spécifier un fichier personnalisé

Utilisez l'option `-f` pour charger un fichier portant un autre nom.

```bash
./minimake -f MonSuperMakefile
```

### 4. S'auto-compiler (Bonus)

Essayez de nettoyer le projet avec GNU Make, puis utilisez votre propre binaire pour reconstruire le projet !

```bash
make clean
./minimake -f Minimakefile
```

## 👥 Auteur

Projet réalisé par :

- Antoine Ramstein

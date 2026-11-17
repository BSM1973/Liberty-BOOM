# BOOM v0.9 — Refonte de l'interface sans boutons superposés

Ce développement remplace intégralement les sources de l'éditeur de la v0.8.
Il conserve les deux images de référence fournies par l'utilisateur. Aucun
bouton visuel supplémentaire n'est placé par-dessus ceux qui sont gravés dans
ces images. Les zones de clic transparentes sont définies dans le seul fichier
Source/BOOMUIMap.h, en coordonnées natives (1672×941 et 1592×988).

## Construction et validation

- 100 presets d'usine, 40 Styles, presets utilisateur avec sauvegarde projet et
  import/export .boom. Les noms/valeurs de sélection sont dynamiques.
- La liste du navigateur et sa zone d'aperçu sont des **contenus** dynamiques,
  pas une seconde couche de boutons.
- Chaque zone de la chaîne décrit un paramètre APVTS réel : champs éditables
  par clic, molette ou glisser ; raccourcis de remise à zéro par double clic.
- Les grands potentiomètres et les cinq bandes utilisent les disques
  photographiques de référence, tournés selon la valeur APVTS.
- Les VU utilisent les mesures audio du moteur DSP.
- Les actions de comparaison, langue, monitoring et bibliothèque ne sont
  pas des paramètres de traitement fictifs.
- Nouveau paramètre « Caractère PUSH » : agit sur la réponse de saturation.
- Nouveau paramètre « Sortie active » : neutralise réellement le gain de sortie.
- Aucun changement de l'identité graphique bordeaux/cuivre demandée.
- Fenêtre fixe : pas d'ascenseur et pas de redimensionnement mal maîtrisé.

### Contrôles automatiques

1. Le test géométrique vérifie toutes les zones de la console et du navigateur
   et échoue sur tout chevauchement.
2. Le contrat UI vérifie que chaque contrôle renvoie à un vrai paramètre JUCE
   ou à un gestionnaire d'action effectivement présent.
3. Les tests DSP, ASan/UBSan, AU/VST3 et les vérifications d'artefacts
   restent obligatoires dans GitHub Actions.
4. Le test DSP supplémentaire vérifie l'activation de l'étage de sortie.

### Essais à effectuer sur le véritable plugin

Ouvrir BOOM dans un hôte AU ou VST3, vérifier le mouvement des 7 boutons,
l'entrée numérique des valeurs de chaque module, les six interrupteurs ACTIF,
la navigation entre 40 Styles et 100 presets, la sauvegarde et le rappel A/B,
le clic des 100 presets et favoris, la mesure VU/LUFS et le chargement après
fermeture/reprise d'un projet. Inspecter le rendu sur Mac 13 pouces.

**Important :** une compilation verte ne prouve pas à elle seule que tous les
gestes sont corrects dans les DAW. Le plafond true-peak est encore en phase de
validation métrologique, distincte de cette refonte UI.

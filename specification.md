# Spécifications — fluoportrait: HDR Portrait Composer

**Nom de travail :** `HDR Portrait Composer`  
**Langage :** C++20  
**Interface graphique :** FLTK 1.4.x  
**Licence suggérée :** BSD 3-Clause  
**Plateformes cibles :** macOS, Linux, Windows  
**Type d’application :** application desktop native légère, sans dépendance à un framework lourd.

## 1. Objectif

Développer une application permettant de créer une image JPEG HDR particulière à partir :

- d’un portrait PNG possédant un canal alpha ;
- d’une couleur de fond choisie par l’utilisateur ;
- d’un profil ICC Rec.2020/PQ nommé :

`Rec2020 Gamut with PQ Transfer`

L’application doit produire une image dans laquelle :

- le fond peut correspondre à une luminance HDR très élevée, typiquement plusieurs centaines ou milliers de nits ;
- le portrait reste visuellement comparable à une image SDR normale ;
- le JPEG final contient le profil ICC Rec.2020/PQ ;
- un navigateur ou logiciel compatible interprète donc le fond comme très lumineux tout en conservant un portrait naturel.

Le principe fondamental est de **ne pas simplement attribuer le profil PQ à une image sRGB existante**.

Le portrait doit être explicitement converti :

**sRGB → RGB linéaire → Rec.2020 linéaire → luminance SDR absolue → PQ/ST2084**

Le fond, lui, est directement défini dans le domaine Rec.2020/PQ.

---

# 2. Cas d’utilisation principal

L’utilisateur :

1. ouvre un fichier PNG ;
2. le PNG contient un portrait et de la transparence alpha autour du sujet ;
3. choisit une couleur de fond ;
4. choisit la luminance HDR du fond ;
5. choisit éventuellement le niveau du blanc SDR utilisé pour le portrait ;
6. visualise :
   - une représentation sans interprétation HDR ;
   - une simulation du résultat Rec.2020/PQ ;
7. enregistre un JPEG ;
8. le JPEG reçoit le profil ICC `Rec2020 Gamut with PQ Transfer`.

---

# 3. Entrées

## 3.1 Image source

Format initial obligatoire :

`PNG`

Le fichier peut être :

- RGB + alpha ;
- RGBA 8 bits ;
- éventuellement RGBA 16 bits dans une version ultérieure.

Version 1 :

**PNG RGBA 8 bits minimum obligatoire.**

Les pixels dont :

`alpha = 0`

sont considérés comme appartenant entièrement au fond.

Les pixels dont :

`alpha = 255`

appartiennent entièrement au portrait.

Les valeurs intermédiaires doivent être compositées correctement.

---

# 4. Gestion du canal alpha

L’alpha doit être utilisé comme coefficient de composition.

Pour :

`a = alpha / 255.0`

le pixel final doit être :

`Final = Portrait × a + Background × (1 - a)`

Mais cette opération doit être réalisée dans un espace approprié.

Il est interdit de réaliser naïvement ce mélange directement entre valeurs sRGB et PQ.

La composition doit intervenir après conversion des deux composantes dans un espace colorimétrique compatible.

Une attention particulière doit être accordée :

- aux contours du crâne ;
- aux poils de barbe ;
- aux cheveux ;
- aux oreilles ;
- aux vêtements ;
- aux pixels alpha partiels.

L’objectif est d’éviter les halos sRGB autour du sujet.

---

# 5. Traitement colorimétrique du portrait

Le portrait PNG doit être considéré par défaut comme :

**sRGB / IEC 61966-2-1**

sauf si un profil ICC incorporé indique explicitement autre chose.

Pour la V1 :

- supporter correctement sRGB ;
- avertir l’utilisateur si une autre source ICC est détectée mais non prise en charge.

---

# 6. Linéarisation sRGB

Chaque composante 8 bits est d’abord normalisée :

`C = value / 255.0`

Puis :

```
if (C <= 0.04045)    C_linear = C / 12.92;
else
    C_linear = pow((C + 0.055) / 1.055, 2.4);
```

Cette opération est appliquée indépendamment à R, G et B.

---

# 7. Conversion sRGB linéaire → Rec.2020 linéaire

Utiliser initialement la matrice suivante :

```
R2020 =    0.6275036748 * R +    0.3292754500 * G +    0.0433026772 * BG2020 =    0.0691083636 * R +    0.9195191578 * G +    0.0113595344 * BB2020 =    0.0163940692 * R +    0.0880112707 * G +    0.8953803700 * B
```

Les entrées sont des valeurs sRGB linéarisées.

Les sorties représentent du **Rec.2020 linéaire relatif**.

Éviter les valeurs négatives :

```
R2020 = std::max(0.0, R2020);
G2020 = std::max(0.0, G2020);
B2020 = std::max(0.0, B2020);
```

Ne pas clamper arbitrairement à `1.0` avant l’encodage HDR si une évolution future permet les valeurs étendues.

---

# 8. Niveau SDR du portrait

La luminance maximale du portrait doit être contrôlable.

Ajouter un paramètre :

**Portrait SDR White**

Valeur par défaut :

`75 nits`

Plage proposée :

`50 – 203 nits`

Valeurs pratiques :

- 50 nits : portrait sombre ;
- 75 nits : recommandé ;
- 100 nits : SDR traditionnel ;
- 203 nits : SDR reference white fréquemment utilisé dans certaines chaînes HDR modernes.

Pour chaque canal :

```
Lr = R2020 × SDRWhiteLg = G2020 × SDRWhiteLb = B2020 × SDRWhite
```

Les valeurs obtenues sont exprimées en **nits**.

---

# 9. Encodage PQ / SMPTE ST2084

Utiliser la véritable fonction ST2084.

Constantes :

```
constexpr double m1 = 2610.0 / 16384.0;
constexpr double m2 = 2523.0 / 32.0;

constexpr double c1 = 3424.0 / 4096.0;
constexpr double c2 = 2413.0 / 128.0;
constexpr double c3 = 2392.0 / 128.0;
```

Fonction :

```
double pqEncode(double luminanceNits){    luminanceNits = std::clamp(luminanceNits, 0.0, 10000.0);    const double L = luminanceNits / 10000.0;    const double p = std::pow(L, m1);    return std::pow(        (c1 + c2 * p) /
        (1.0 + c3 * p),        m2
    );}
```

Résultat :

`0.0 … 1.0`

Conversion JPEG 8 bits :

```
uint8_t value =    static_cast<uint8_t>(        std::round(            std::clamp(pq, 0.0, 1.0) * 255.0
        )    );
```

---

# 10. Quelques valeurs PQ de référence

L’interface pourra utiliser ces valeurs pour les tests :

| luminance | PQ 8 bits approx. |
| --------- | ----------------- |
| 18 nits   | 89                |
| 25 nits   | 96                |
| 35 nits   | 104               |
| 50 nits   | 112               |
| 75 nits   | 122               |
| 100 nits  | 130               |
| 203 nits  | 148               |
| 1000 nits | 192               |
| 1300 nits | 199               |

Ces valeurs servent uniquement de repères.

La fonction ST2084 doit rester la source de vérité.

---

# 11. Fond HDR

L’utilisateur doit pouvoir choisir une couleur.

Deux méthodes doivent idéalement être proposées.

## Mode A — RGB PQ direct

L’utilisateur saisit :

```
R = 196G = 202B = 156
```

Ces valeurs sont considérées directement comme des **codes PQ Rec.2020 8 bits**.

C’est particulièrement utile pour reproduire une image existante.

Exemple connu donnant un fond extrêmement lumineux :

```
196 / 202 / 156
```

---

# 12. Mode B — Couleur + luminance en nits

Mode recommandé.

L’utilisateur choisit :

- teinte ;
- saturation ;
- luminance souhaitée.

Par exemple :

```
Hue: lime greenLuminance: 1200 nits
```

Le logiciel calcule ensuite les valeurs Rec.2020/PQ correspondantes.

L’interface doit présenter simultanément :

```
Background RGB PQ :  xxx / xxx / xxxEstimated luminance: xxxx nits
```

---

# 13. Calcul de la luminance du fond

Pour des valeurs **Rec.2020 linéaires**, utiliser :

```
Y =0.2627 × R +0.6780 × G +0.0593 × B
```

`Y` représente la luminance relative.

Après conversion en luminance absolue :

```
nits = Y × référence
```

Pour des codes PQ déjà existants, il faudra d’abord effectuer l’opération inverse :

**PQ → luminance absolue ST2084**

puis calculer la luminance Rec.2020 correspondante.

---

# 14. Décodage PQ

Ajouter la fonction inverse.

```
double pqDecode(double N){    constexpr double m1 = 2610.0 / 16384.0;    constexpr double m2 = 2523.0 / 32.0;    constexpr double c1 = 3424.0 / 4096.0;    constexpr double c2 = 2413.0 / 128.0;    constexpr double c3 = 2392.0 / 128.0;    N = std::clamp(N, 0.0, 1.0);    const double p = std::pow(N, 1.0 / m2);    const double numerator =        std::max(p - c1, 0.0);    const double denominator =        c2 - c3 * p;    if (denominator <= 0.0)        return 10000.0;    return
        10000.0 *
        std::pow(            numerator / denominator,            1.0 / m1
        );}
```

---

# 15. Point essentiel : ne pas modifier le fond pour corriger le portrait

Le fond et le portrait doivent suivre **deux pipelines séparés**.

```
Portrait sRGB    ↓linear sRGB    ↓linear Rec.2020    ↓SDR white = 75 nits    ↓PQ    │    ├──────┐           ↓ composition    ┌──────┘Background Rec.2020/PQ
```

Le choix du niveau SDR du portrait ne doit **jamais modifier le fond**.

Inversement, modifier la luminance du fond ne doit pas modifier le portrait.

---

# 16. Correction facultative de la peau

Même après une conversion physiquement correcte, proposer une correction optionnelle destinée aux portraits.

Paramètres :

```
Skin Red CompensationPortrait SaturationPortrait Exposure
```

Valeurs par défaut :

```
Skin Red Compensation : 0 %Portrait Saturation    : 100 %Portrait Exposure      : 0 EV
```

Ne jamais appliquer automatiquement une division arbitraire du canal rouge.

Une correction éventuelle pourrait utiliser un mélange doux :

```
R' = (1-k)R + kG
```

avec par exemple :

`0 ≤ k ≤ 0.20`

Le réglage doit rester facultatif.

---

# 17. Interface utilisateur FLTK

Fenêtre principale proposée :

```
+----------------------------------------------------------+| HDR Portrait Composer                                    |+----------------------------------------------------------+| File: portrait.png                     [ Open... ]        ||                                                          || +----------------------+  +----------------------------+ || | RAW / SDR PREVIEW    |  | HDR/PQ PREVIEW             | || |                      |  |                            | || |                      |  |                            | || +----------------------+  +----------------------------+ ||                                                          || Background                                               || [color swatch] [ Choose Color... ]                       ||                                                          || Background mode:                                         || ( ) PQ RGB Codes                                         || ( ) Color + Nits                                         ||                                                          || R [196]  G [202]  B [156]                                ||                                                          || Estimated luminance : 1304 nits                          ||                                                          || Portrait SDR white : [ 75 ] nits                         ||                                                          || Portrait exposure  : [ 0.0 ] EV                          || Portrait saturation: [100 ] %                            || Skin red correction: [ 0 ] %                             ||                                                          || ICC profile : Rec2020 Gamut with PQ Transfer             || Profile status : Loaded                                  ||                                                          ||                             [ Export JPEG... ]            |+----------------------------------------------------------+
```

---

# 18. Première vue : RAW / non-HDR

La vue de gauche doit montrer une représentation volontairement simple des **codes RGB du JPEG final sans appliquer le profil PQ**.

Elle permet de comprendre pourquoi le JPEG peut paraître relativement terne lorsqu’il est interprété comme RGB classique.

Cette vue doit être explicitement identifiée comme :

`Raw RGB codes / no PQ interpretation`

Elle ne prétend pas représenter une image colorimétriquement correcte.

---

# 19. Deuxième vue : simulation HDR/PQ

La vue de droite doit fournir une approximation de l’apparence finale.

Problème :

FLTK lui-même ne fournit pas nécessairement un pipeline HDR natif uniforme sur toutes les plateformes.

La V1 peut donc utiliser une **simulation tone-mapped SDR**.

Elle doit :

1. décoder PQ vers nits ;
2. appliquer une courbe de tone mapping configurable ;
3. convertir vers l’espace d’affichage ;
4. produire une preview SDR.

Ajouter clairement :

`HDR/PQ simulation`

et non :

`exact HDR preview`

si l’environnement ne permet pas d’affichage HDR natif.

---

# 20. Option très utile : Browser Preview

Ajouter éventuellement un bouton :

`Open Browser Preview`

Fonctionnement :

1. exporter temporairement le JPEG final ;
2. générer une petite page HTML locale ;
3. ouvrir cette page dans le navigateur par défaut.

Exemple :

```
<!doctype html>
<html>
<head>
<meta charset="utf-8">
<style>
html, body {    margin: 0;    background: #777;    height: 100%;}

body {    display: flex;    justify-content: center;    align-items: center;}

img {    max-width: 90vw;    max-height: 90vh;}
</style>
</head>

<body>
<img src="preview.jpg">
</body>
</html>
```

Cette fonctionnalité est particulièrement intéressante puisque le but du logiciel est précisément de produire des JPEG destinés aux navigateurs.

---

# 21. Gestion du profil ICC

Le logiciel doit pouvoir utiliser un fichier ICC externe :

`Rec2020 Gamut with PQ Transfer.icc`

Ne pas dépendre du nom du fichier.

Identifier le profil à partir de son contenu ICC lorsque possible.

L’application doit afficher :

```
ICC profile loaded:Rec2020 Gamut with PQ Transfer
```

L’utilisateur doit pouvoir sélectionner un autre fichier ICC :

`Preferences → HDR ICC Profile`

---

# 22. Ne pas embarquer automatiquement un profil tiers

Pour éviter tout problème de redistribution/licence, l’architecture doit permettre :

- soit de fournir un profil ICC librement redistribuable ;
- soit à l’utilisateur de sélectionner son propre profil ;
- soit d’extraire le profil depuis une image JPEG de référence.

Fonction optionnelle particulièrement intéressante :

`Import ICC profile from JPEG...`

Elle extrait le segment ICC APP2 d’un JPEG compatible.

---

# 23. Encapsulation ICC dans le JPEG

Le JPEG final doit contenir le profil ICC.

Les profils ICC dans JPEG utilisent normalement un ou plusieurs segments **APP2**.

Signature :

```
ICC_PROFILE\0
```

Le profil peut dépasser la taille d’un segment JPEG et doit alors être découpé conformément au mécanisme ICC/JPEG.

Ne pas simplement concaténer arbitrairement les données.

---

# 24. Bibliothèque JPEG

Choix recommandé :

**libjpeg-turbo**

Motifs :

- C/C++ ;
- mature ;
- rapide ;
- multiplateforme ;
- très répandue.

Cependant l’injection du profil ICC devra être vérifiée soigneusement.

Créer une abstraction :

```
class JpegWriter
{
public:    bool write(        const std::filesystem::path& output,        const ImageRGB8& image,        const ICCProfile& profile,        int quality
    );};
```

---

# 25. Chargement PNG

Bibliothèque recommandée :

**libpng**

ou éventuellement :

**stb_image**

Pour un logiciel colorimétrique sérieux, libpng est préférable car les métadonnées sont plus facilement contrôlables.

Structure interne :

```
struct RGBA8
{    uint8_t r;    uint8_t g;    uint8_t b;    uint8_t a;};
```

---

# 26. Représentation interne

Ne pas effectuer les calculs colorimétriques en `uint8_t`.

Utiliser au minimum :

```
struct RGBf
{    double r;    double g;    double b;};
```

Les calculs colorimétriques sont réalisés en `double`.

La quantification en 8 bits intervient uniquement à la fin.

---

# 27. Multithreading

Le traitement est parfaitement parallélisable par lignes.

Architecture suggérée :

```
ImageProcessor    ├── Worker 1 : rows 0–n    ├── Worker 2    ├── Worker 3    └── Worker N
```

Utiliser :

```
std::thread
```

ou :

```
std::jthread
```

C++20.

Ne jamais bloquer l’interface FLTK pendant une conversion importante.

---

# 28. Classes suggérées

```
ApplicationMainWindowImageViewPreviewWidgetPNGLoaderJpegWriterICCProfileColorSciencePQTransferSRGBTransferRec2020PortraitProcessorBackgroundGeneratorCompositorProjectPreferences
```

---

# 29. ColorScience

Exemple d’API :

```
namespace ColorScience
{    RGBf srgbToLinear(const RGBf&);    RGBf linearSRGBToRec2020(const RGBf&);    double pqEncode(double nits);    double pqDecode(double code);    RGBf portraitToPQ(        const RGBf& srgb,        double sdrWhiteNits
    );    double rec2020Luminance(        const RGBf& linearRec2020
    );}
```

Cette partie doit être indépendante de FLTK et entièrement testable en ligne de commande.

---

# 30. Séparation GUI / moteur

Impératif :

**aucun calcul colorimétrique dans les callbacks FLTK.**

Architecture :

```
GUI ↓Project model ↓Image processing engine ↓Color science
```

Cela permettra ultérieurement :

- une CLI ;
- des tests automatisés ;
- du batch ;
- éventuellement des bindings.

---

# 31. Mise à jour interactive

Lorsque l’utilisateur change :

- couleur ;
- nits ;
- SDR white ;
- exposition ;
- saturation ;

la preview doit être recalculée.

Pour éviter une mauvaise réactivité :

- preview basse résolution pendant les modifications ;
- traitement pleine résolution uniquement lors de l’export.

Par exemple :

`preview maximum = 1024 × 1024`

---

# 32. Dithering

L’encodage final n’est que 8 bits par canal.

Le PQ peut rendre certaines transitions particulièrement sensibles.

Prévoir un dithering facultatif avant quantification :

- ordered dithering ;
- ou bruit triangulaire très faible.

Option :

`8-bit dithering`

activée par défaut.

Le bruit ne doit pas affecter les zones uniformes au point de rendre le fond visiblement granuleux.

---

# 33. Export

Dialogue :

```
JPEG Quality       [95]ICC Profile        Rec2020 Gamut with PQ TransferDithering          [x]Optimize JPEG      [x][Cancel] [Export]
```

Qualité par défaut :

`95`

---

# 34. Métadonnées

Version 1 :

- conserver uniquement le profil ICC ;
- pas besoin de copier EXIF ;
- pas besoin de GPS ;
- pas besoin de XMP.

Cela produit un JPEG relativement propre :

```
JPEGJFIFICC APP2 Rec.2020/PQimage data
```

---

# 35. Vérification après export

Après écriture, le logiciel doit pouvoir rouvrir automatiquement le JPEG et vérifier :

```
JPEG valid               YESICC embedded             YESICC description          Rec2020 Gamut with PQ Transferdimensions               WxHRGB                       YES
```

En cas d’échec :

ne pas annoncer que l’export est réussi.

---

# 36. Informations techniques affichables

Ajouter éventuellement un panneau :

`Image Information`

Exemple :

```
Source------1024 × 1024RGBA 8-bitsRGBAlpha: yesPortrait--------SDR white: 75 nitsExposure: 0 EVBackground----------PQ code: 196 / 202 / 156Approx luminance: 1304 nitsOutput------JPEG 8-bit RGBICC: Rec2020 Gamut with PQ TransferTransfer: ST2084 / PQPrimaries: Rec.2020
```

---

# 37. Presets

Prévoir quelques presets.

### Original yellow HDR

```
RGB PQ:196202156
```

### Lime HDR

Preset dérivé expérimentalement du résultat vert obtenu.

### SDR Portrait

```
SDR white:75 nits
```

Permettre :

`Save preset…`

---

# 38. Comparaison avant/après

Ajouter un mode :

`Split View`

Avec curseur vertical :

```
RAW | PQ simulated
```

Cela aidera énormément à comprendre l’effet.

---

# 39. Tests unitaires

Obligatoires pour `ColorScience`.

Utiliser par exemple :

**Catch2**

Tests minimum :

### sRGB black

```
0,0,0→0,0,0
```

### sRGB white

```
255,255,255
```

avec `SDRWhite = 100 nits`

doit aboutir approximativement au code PQ correspondant à :

`100 nits ≈ 130/255`

après conversion colorimétrique.

### PQ

```
pqEncode(100.0)≈ 0.508078
```

### PQ

```
pqEncode(1000.0)≈ 0.751827
```

### Round trip

```
pqDecode(pqEncode(L))≈ L
```

pour :

```
0.111050751002031000400010000
```

---

# 40. Test fond de référence

Créer un test particulier pour :

```
RGB PQ:196 / 202 / 156
```

Le logiciel doit être capable d’afficher la luminance calculée correspondante et de reproduire exactement les trois octets lors de l’export en mode :

`Direct PQ RGB`

Il ne doit appliquer **aucune conversion supplémentaire** à ces trois valeurs.

---

# 41. Test alpha

Image test :

```
opaque white50 % alphatransparent
```

Vérifier la composition des trois cas.

Le bord semi-transparent ne doit produire :

- ni halo noir ;
- ni halo blanc ;
- ni halo sRGB.

---

# 42. Important : profil ICC ≠ conversion

Le logiciel doit clairement distinguer :

### Assign profile

Changer uniquement l’interprétation colorimétrique.

### Convert

Transformer réellement les pixels.

Dans notre pipeline final, les pixels sont déjà **explicitement produits en Rec.2020/PQ**.

Le profil ICC est donc ensuite **attaché** au JPEG pour déclarer correctement ce que représentent les codes numériques.

Il ne faut pas refaire une conversion ICC lors de l’écriture qui modifierait les valeurs calculées.

C’est un point critique.

---

# 43. Critère essentiel de fidélité

Pour un fond en mode direct :

```
196 / 202 / 156
```

une analyse du JPEG décodé doit retrouver approximativement ces valeurs, modulo les très faibles artefacts de compression JPEG.

Une transformation du genre :

```
196,202,156→250,255,180
```

est un bug.

---

# 44. Compression JPEG et fond uniforme

La compression JPEG peut légèrement altérer le fond.

Pour minimiser cela :

- qualité 95–100 ;
- idéalement chroma subsampling configurable.

Prévoir :

```
4:4:44:2:24:2:0
```

Pour ce type d’image :

**4:4:4 recommandé.**

---

# 45. CLI future

Préparer le moteur pour permettre plus tard :

```
hdrportrait \    input.png \    --background-pq 196,202,156 \    --sdr-white 75 \    --icc rec2020-pq.icc \    --quality 95 \    -o output.jpg
```

La GUI ne doit donc pas être nécessaire au moteur.

---

# 46. Fichier projet

Extension proposée :

`.hdrportrait`

Format JSON.

Exemple :

```
{    "source": "portrait.png",    "background": {        "mode": "pq_rgb",        "r": 196,        "g": 202,        "b": 156
    },    "portrait": {        "sdr_white_nits": 75.0,        "exposure_ev": 0.0,        "saturation": 1.0,        "skin_red_correction": 0.0
    },    "jpeg": {        "quality": 95,        "subsampling": "444"
    }}
```

---

# 47. Portabilité

Cibles obligatoires :

### macOS

- Intel
- Apple Silicon

### Linux

- x86-64
- éventuellement ARM64

### Windows

- x86-64

Build :

**CMake**

---

# 48. Dépendances souhaitées

Minimiser les dépendances.

```
FLTK 1.4.xlibpnglibjpeg-turboLittleCMS2 éventuellementCatch2 pour tests
```

LittleCMS ne doit cependant **pas remplacer les transformations mathématiques PQ explicitement définies** sans validation préalable.

Le pipeline doit rester compréhensible et déterministe.

---

# 49. Organisation du repository

```
HDRPortraitComposer/├── CMakeLists.txt├── LICENSE├── README.md├── docs/│   ├── color-pipeline.md│   └── architecture.md├── src/│   ├── main.cpp│   ├── app/│   ├── gui/│   ├── image/│   ├── color/│   └── io/├── include/├── tests/│   ├── test_pq.cpp│   ├── test_srgb.cpp│   ├── test_rec2020.cpp│   └── test_compositor.cpp└── resources/
```

---

# 50. Priorités de développement pour Codex

Codex doit travailler dans cet ordre :

**Phase 1 — moteur mathématique**

- ST2084 encode/decode ;
- sRGB linearisation ;
- conversion sRGB → Rec.2020 ;
- tests.

**Phase 2 — image**

- lecture PNG RGBA ;
- transformation portrait ;
- génération background ;
- alpha compositing.

**Phase 3 — JPEG**

- libjpeg-turbo ;
- ICC APP2 ;
- vérification après écriture.

**Phase 4 — CLI de validation**

Créer temporairement une CLI avant l’interface.

Cela permet de tester :

```
input.png → output.jpg
```

dans Chrome/Firefox/Safari.

**Phase 5 — FLTK**

Construire l’interface seulement lorsque le pipeline produit déjà les bons JPEG.

**Phase 6 — previews**

- raw codes ;
- simulation HDR ;
- browser preview.

---

# 51. Critères d’acceptation de la V1

La V1 est considérée comme fonctionnelle uniquement si :

1. un PNG RGBA peut être ouvert ;
2. son alpha est respecté ;
3. une couleur de fond PQ peut être définie ;
4. sa luminance estimée est affichée ;
5. le portrait est converti de sRGB vers Rec.2020/PQ avec un blanc SDR configurable ;
6. le fond et le portrait restent réglables indépendamment ;
7. un JPEG RGB 8 bits peut être exporté ;
8. le JPEG contient effectivement le profil ICC ;
9. le profil est identifié comme `Rec2020 Gamut with PQ Transfer` ;
10. le fichier peut être ouvert dans un navigateur ;
11. le fond HDR conserve son effet lumineux ;
12. le portrait reste visuellement naturel ;
13. les codes PQ directs du fond ne sont pas modifiés par une conversion ICC intempestive.

---

# 52. Principe à ne jamais perdre de vue

Le logiciel ne cherche pas à rendre **toute l’image HDR**.

Il cherche volontairement à créer deux régimes de luminance dans le même fichier :

```
PORTRAIT≈ SDR≈ 0–75 nits        │        │ énorme contraste de luminance        ▼BACKGROUND≈ HDR≈ plusieurs centaines à >1000 nits
```

Les deux sont ensuite encodés dans le **même espace Rec.2020/PQ**.

C’est cette différence de luminance absolue qui produit le résultat recherché.

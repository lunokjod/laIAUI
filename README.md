# laIAUI - Interfície Gràfica d'IA amb Execució de Comandes

Una aplicació GUI moderna escrita en C++ que combina un assistent d'IA amb capacitat per executar comandes de terminal en temps real.

## Característiques Principals

### 🤖 **Assistent d'IA Integrat**
- Connexió amb l'API de DeepSeek
- Streaming de respostes en temps real
- Historial de conversa persistent
- Interfície en català per defecte

### 💻 **Execució de Comandes**
- Execució segura de comandes de terminal GNU/Linux
- Visualització en temps real de stdout/stderr
- Resultats col·lapsables amb detalls complets
- Canvi de directori de treball

### 🎨 **Interfície Gràfica Moderna**
- Basada en ImGui + GLFW + OpenGL 3.3
- Disseny arrodonit i atractiu
- Colors diferenciats per tipus de missatge
- Auto-scroll i enfocament automàtic
- Menú contextual per copiar el xat sencer

### ⚡ **Funcionament en Temps Real**
- Streaming de respostes de l'IA
- Execució asíncrona de comandes
- Gestió de múltiples tasques simultànies
- Indicador visual d'estat (Online/Thinking/Offline)

## Estructura del Codi

### Classes Principals

1. **`TerminalEmulator`**
   - Execució segura de comandes de terminal
   - Gestió de directori de treball
   - Captura de stdout/stderr

2. **`DeepSeekClient`**
   - Client per a l'API de DeepSeek
   - Suport per streaming amb tool calls
   - Gestió d'historial de conversa

3. **`AsyncTaskManager`**
   - Gestió de tasques asíncrones
   - Cua de missatges per streaming
   - Callbacks per actualitzacions en temps real

4. **`ChatApplication`**
   - Lògica principal de l'aplicació
   - Gestió d'estat de la GUI
   - Processament de missatges i resultats

### Components de la GUI

- **Àrea de Xat**: Mostra la conversa amb colors diferenciats
- **Entrada de Text**: Multilínia amb suport per tabulacions
- **Resultats de Comandes**: Visualització col·lapsable amb detalls
- **Barra de Menú**: Opcions de neteja i sortida
- **Indicador d'Estat**: Visualització de l'estat de connexió

## Requisits del Sistema

### Dependències
- **CMake** (>= 3.10)
- **GLFW3** (per a la finestra i gestió d'esdeveniments)
- **OpenGL** (>= 3.3)
- **cURL** (per a connexions HTTP)
- **nlohmann/json** (per a processament JSON)
- **ImGui** (inclòs com a submodule)

### Variables d'Entorn
```bash
export DEEPSEEK_API_KEY="la_teva_clau_api_aquí"
```

## Compilació i Execució

### Compilació
```bash
# Clonar el repositori
git clone https://github.com/tu-usuari/laIAUI.git
cd laIAUI

# Inicialitzar submodules
git submodule update --init --recursive

# Crear directori de compilació
mkdir build && cd build

# Configurar amb CMake
cmake ..

# Compilar
make
```

### Execució
```bash
# Assegurar-se que la clau API està configurada
export DEEPSEEK_API_KEY="la_teva_clau_api"

# Executar l'aplicació
./laIAUI
```

## Ús de l'Aplicació

### Inici de Conversa
1. Inicia l'aplicació
2. Escriu el teu missatge a l'àrea de text inferior
3. Prem Enter o fes clic a "Send"

### Execució de Comandes
L'IA pot executar comandes de terminal automàticament quan ho consideri necessari. Els resultats es mostren com a elements col·lapsables que inclouen:
- Explicació de la comanda
- Sortida estàndard (stdout)
- Errors (stderr)
- Directori actual
- Estat i codi de retorn

### Funcions Especials
- **Ctrl+N**: Netejar el xat
- **Ctrl+Q**: Sortir de l'aplicació
- **Clic dret a l'àrea de xat**: Copiar tot el xat
- **Checkbox "Tools"**: Activar/desactivar execució de comandes

## Estructura de Projecte
```
laIAUI/
├── main.cpp              # Codi font principal
├── CMakeLists.txt        # Configuració de CMake
├── README.md            # Aquest fitxer
├── imgui/               # Submodule d'ImGui
└── build/               # Directori de compilació
```

## Configuració de la GUI

### Estils
- Finestres arrodonides (10px)
- Botons arrodonits (8px)
- Colors personalitzats per tipus de missatge:
  - **Usuari**: Blau cian
  - **IA**: Groc verdós
  - **Sistema**: Verd clar
  - **Comandes**: Gris amb detalls expandibles

### Layout Responsiu
- Redimensionament automàtic
- Àrea de xat ajustable
- Entrada de text multilínia
- Botons adaptatius

## Gestió d'Errors

### Errors de Connexió
- Missatge clar quan falta la clau API
- Indicador "Offline" a la barra de menú
- Suggeriment per configurar la variable d'entorn

### Errors d'Execució
- Visualització de stderr en vermell
- Codi de retorn visible
- Estat de fallada clarament indicat

## Seguretat

### Execució de Comandes
- Execució en el directori de treball actual
- Captura separada de stdout/stderr
- Limitació de permisos (executa com a l'usuari actual)

### Gestió de Memòria
- Ús de smart pointers (unique_ptr)
- Neteja adequada de recursos
- Gestió d'excepcions

## Personalització

### Modificació del Prompt del Sistema
Edita la línia 490 de `main.cpp`:
```cpp
string systemPrompt = "Ets un assistent AI útil que parla català. Pots executar comandes de terminal quan sigui necessari.";
```

### Canvi d'API
Modifica el constructor de `DeepSeekClient` (línia 290) per utilitzar un altre endpoint.

## Contribucions

Les contribucions són benvingudes! Si us plau:

1. Fes un fork del projecte
2. Crea una branca per a la teva característica
3. Fes commit dels teus canvis
4. Fes push a la branca
5. Obre un Pull Request

## Llicència

Aquest projecte està sota la llicència MIT. Consulta el fitxer `LICENSE` per a més detalls.

## Agraïments

- **ImGui**: Per la fantàstica llibreria d'interfície immediata
- **DeepSeek**: Per l'API d'IA accessible
- **GLFW**: Per la gestió multiplataforma de finestres
- **Tots els contribuïdors**: Per fer possible aquest projecte

## Contacte

Per a preguntes o suport:
- Obre un issue al GitHub del projecte
- Contacta amb el mantenedor principal

---

**Nota**: Aquesta aplicació requereix una clau API vàlida de DeepSeek per funcionar. Assegura't de configurar la variable d'entorn `DEEPSEEK_API_KEY` abans d'executar l'aplicació.# laIAUI

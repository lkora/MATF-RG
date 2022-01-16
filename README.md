# MATF-RG - Snail Island
______________________
Projekat iz Računarske grafike, korišćen skelet sa `https://github.com/matf-racunarska-grafika/project_base.git`. <br>
Rađeno uz pomoć materijala sa [LearnOpenGL.com](https://learnopengl.com/)

## Uputstvo pre izvršavanja
1. `git clone https://github.com/lkora/MATF-RG.git`
2. CLion -> Open -> path/to/my/project_base
3. Main se nalazi u src/main.cpp
4. Cpp fajlovi idu u src folder
5. Zaglavlja (h i hpp) fajlovi idu u include
6. Šejderi idu u folder shaders. `Vertex shader` ima ekstenziju `.vs`, `fragment shader` ima ekstenziju `.fs`
7. ALT+SHIFT+F10 -> project_base -> run

## Uputstvo tokom izvršavanja
- Kretanje u prostoru uz pomoć miša i tastature:
  - `ESC` - prekida izvršavanje programa
  - `W` - napred u odnosu na smer pogleda kamere
  - `S` - unazad u odnosu na smer pogleda kamere
  - `A` - kretanje u levu stranu
  - `D` - kretanje u desnu stranu
  - `MWHEELUP` - sužavanje vidnog polja "manji FOV"
  - `MWHEELDOWN` - proširavanje vidnog polja "veći FOV"
- Funkcionalnosti:
  - `F1` - otvara ImGUI sa svim opcijama
    <br>Sastoji se od 3 dela koje je moguće aktivirati i deaktivirati uz pomoć checkboxova na početku glavne forme. <br>Umesto ImGUI-a za kontrolu nekih opcija moguće je koristiti sledeće skraćenice za opcije:
    - `C` - Toggle zakljućavanje i otključavanje kamere
    - `L` - Toggle između Blinn-Phong i Phong Lighting modela 
    - `H` - Toggle aktiviranje i deaktiviranje HDR efekta
    - `B` - Toggle aktiviranje i deaktiviranje Bloom efekta
    
## Opis
### Korišćeni Aseti
Svi korišćeni aseti se mogu naći u folderu `resources`, sve je pravljeno za projekat osim:
- [Puž sa kućom](https://free3d.com/3d-model/snail-with-toy-house-for-shell-v2--598985.html)
- [Cubemap HDR](https://cgtricks.com/free-high-quality-space-hdri-for-your-art-and-creativity/)
### Osvetljenje scene
Sadrži jedno direkciono i 5 tačkastih izvora svetlosti, moguće je menjati usmerenje direkcionog osvetljenja kao i sve ostale parametre svetlima, osim pozicija tačkastih svetala kroz ImGUI.
### Implementirane oblasti
- Obavezne oblasti:
  - [x] Od 1. do 8. nedelje
  - [x] [Blending](https://learnopengl.com/Advanced-OpenGL/Blending) - u teksturi oblaka
  - [x] [Face Culling](https://learnopengl.com/Advanced-OpenGL/Face-culling)
  - [x] [Advanced Lighting](https://learnopengl.com/Advanced-Lighting/Advanced-Lighting) - Blinn-Phong i Phongov model osvetljenja sa mogućom promenom tokom izvršavanja programa
- Kategorija A:
  - [x] [Framebuffers](https://learnopengl.com/Advanced-OpenGL/Framebuffers)
  - [x] [Cubemaps](https://learnopengl.com/Advanced-OpenGL/Cubemaps)
  - [x] [Instancing](https://learnopengl.com/Advanced-OpenGL/Instancing) - oblaci
  - [ ] [Anti Aliasing](https://learnopengl.com/Advanced-OpenGL/Anti-Aliasing)
- Kategorija B:
  - [ ] [Point Shadows](https://learnopengl.com/Advanced-Lighting/Shadows/Point-Shadows)
  - [ ] [Normal mapping](https://learnopengl.com/Advanced-Lighting/Normal-Mapping) // TODO
  - [ ] [Parallax Mapping](https://learnopengl.com/Advanced-Lighting/Parallax-Mapping)  // TODO
  - [x] [HDR](https://learnopengl.com/Advanced-Lighting/HDR)
  - [x] [Bloom](https://learnopengl.com/Advanced-Lighting/Bloom)
  - [ ] [Deffered Shading](https://learnopengl.com/Advanced-Lighting/Deferred-Shading)
  - [ ] [SSAO](https://learnopengl.com/Advanced-Lighting/SSAO)

______________
## Detalji o kursu
Seminarski rad 2021/2022<br>
Asistent: Marko Spasić <br>
Profesor: dr. Vesna Marinković

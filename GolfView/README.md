

#  GolfView - DIY dashboard


## 💡 Wstęp (Introduce)

Jako posiadacz Golfa IV z 2002 postanowiłem wykonać projekt, który pozwoli mi wychwycić dane niedostępne z pozycji kokpitu, przetworzyć je i wyświetlać na dodatkowym ekranie. Oryginalny dashboard poza kontrolkami i kilometrażem wyświetla tylko prędkość, RPM, poziom paliwa i temperaturę oleju lecz brakuje na nim np. spalania, średniej prędkości czy innych ważnych informacji. Czy uda się to poprawić w domu tanim kosztem? To się okaże, do dzieła!


## 📏Planowanie (Planning)

Dane będę czytał z ECU z pomocą adaptera **ELM327** do **OBD-II** z interfejsem **Bluetooth** do komunikacji, a „mózgiem” operacji zostanie płytka **ESP32 DevKit v1**, której dużą zaletą jest wbudowany Bluetooth. Dodatkowo zależało mi na zasilaniu **po zapłonie** dla tego projektu (oraz kamerki), ponieważ gniazdo zapalniczki podaje prąd cały czas — dlatego zdecydowałem się wykorzystać wyjście z **przekaźnika odciążającego nr 100** (szyna **75X**), który podaje **+12 V** po przekręceniu kluczyka (ale **odcina** je podczas rozruchu). Dalej użyję przetwornicy **step-down LM2596**, która bezpiecznie może dostarczyć do **2 A** na wyjściu (w typowym module). **Bezpiecznik** na linii **+12 V** przed przetwornicą dobiorę z odpowiednim zapasem na podstawie poniższych obliczeń.

<details>
  <summary>EN</summary>


I will read data from the ECU using an **ELM327** **OBD-II** adapter with **Bluetooth** for communication, and the brain of the system will be an **ESP32 DevKit v1**, which conveniently has built-in Bluetooth. I also wanted **ignition-switched power** for this project (and the dash cam), since the cigarette lighter is permanently live — so I decided to use the output from the **load-reduction relay No.100** (the **75X** bus), which supplies **+12 V** when the key is in the **ON** position (but **cuts it** during cranking). Downstream I’ll use an **LM2596 step-down** converter, which can safely provide up to **2 A** at the output (typical module). I’ll select the **fuse** on the **+12 V** line before the converter with a proper margin based on the calculations below.
</details>


**1. Bilans prądu dla wyjścia przetwornicy dla realnych wartości prądu w odbiornikach (Current draw budget at the converter output):**

$$
\begin{aligned}
I_{\mathrm{OLED}}      &= 100\ \mathrm{mA} \\
I_{\mathrm{IDUINO}}    &= 5\ \mathrm{mA} \\
I_{\mathrm{ESP32}}     &= 200\ \mathrm{mA} \\
I_{\mathrm{dash\ cam}} &= 800\ \mathrm{mA} \\
I_{\mathrm{OUT}}       &= 100 + 5 + 200 + 800 = 1105\ \mathrm{mA} \approx 1.1 < 2A\ \mathrm{A}
\end{aligned}
$$

<div align="center">
LM2596 będzie OK (LM2596 will be OK)✅
<p></p>
</div>

**2. Wybór bezpiecznika (fuse choice)**

<div align="center">
<p></p>

$\eta \approx 0.88$, step-down from $V_{\text{in}}=14.5\,\mathrm{V}$ to $V_{\text{out}}=5\,\mathrm{V}$, and $I_{\text{OUT,max}}=2\,\mathrm{A}$:


$$
\eta \cdot P_{\text{IN}} = P_{\text{OUT}}
$$

$$
\eta \cdot I_{\text{IN max}} \cdot V_{\text{IN}} = I_{\text{OUT max}} \cdot V_{\text{OUT}}
$$

$$
I_{\text{IN max}} = \frac{I_{\text{OUT max}} \cdot V_{\text{OUT}}}{\eta \cdot V_{\text{IN}}}
= \frac{2 \cdot 5}{0.88 \cdot 14.5}
\approx \mathbf{0.78\mathrm{A}}
$$

Zatem biorąc pod uwagę zapas zastosuję bezpiecznik 1A szklany (//)
<p></p>
  
</div>


## 🔌Schemat

Finalnie rozważyłem taki schemat:
<details>
  <summary>EN</summary>
  hujhujhuj
</details>

<img src=https://github.com/user-attachments/assets/c0414036-ffb5-4203-a85c-da5e9e6c3d69>
Wstawka z przyszłości: Na tym etapie świadomie nie dodawałem zewnętrznego filtra LC: moduł LM2596 ma bazową filtrację, a pomiary i testy drogowe nie wykazały zakłóceń (resetów ESP32/artefaktów OLED). Zostawiłem miejsce na dławik 10–22 µH i kondensator 470–1000 µF na przyszłość — dołożę je tylko, jeśli pojawią się problemy w eksploatacji.


## 🛠Budowa
Budowę całego układu rozpocząłem od zaprojektowania w Autodesk Inventor obudowy na ESP32 razem z miejscem na joystick. Wymiary dopasowałem do wolnej półki DIN1 w moim aucie, oto krótka animacja 3-częściowej, najważniejszej obudowy (funfact: generowała się w inventor studio aż 7h). Obudowę wydrukowałem z PLA na moim Bambu Lab A1 mini (gif a ). 
Kolejnymi obudowami jakie wydrukowałem są kolejno obudowa na OLED (3 części) oraz obudowa na PCB THT (który zlutowałem by prosto rozdzielić zasilanie 5V na kilka odbiorników za pomocą złącz śrubowych, gif b):
<details>
  <summary>EN</summary>
  hujhujhuj
</details>

<div align="center">
  <img src="https://github.com/user-attachments/assets/fcec71b2-29f3-4b15-aafa-8b4e60a39a53" width="400">
</div>


Kolejnymi obudowami jakie wydrukowałem są kolejno obudowa na OLED (3 części) oraz obudowa na PCB THT (który zlutowałem by prosto rozdzielić zasilanie 5V na kilka odbiorników za pomocą rasterów):
<details>
  <summary>EN</summary>
  hujhujhuj
</details>
<div align="center">
  <img src="https://github.com/user-attachments/assets/9611d9d1-51a3-49a0-ad28-ab50cbb9df87" width="400">
</div>





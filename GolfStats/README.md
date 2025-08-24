
<p align="center">
  <img src="https://github.com/user-attachments/assets/b6d75f08-d0ef-4bcd-9987-e7be7c5bd869" 
       width="100%" 
       height="250"
       style="object-fit: cover; object-position: center;" />
</p>

# 🚗 GolfStats - czyli od płytki i OLED do pomocnego interfejsu w samochodzie


## 💡 Wstęp

Jako dumny posiadacz Golfa IV z 2002 roku w dosyć biednej konfiguracji jeśli chodzi o wskaźniki, amator i początkujący zajawkowicz elektroniki, druku 3D i motoryzacji postanowiłem wykonać projekt, który pozwoli mi wyświetlać dane niedostępne dla oka z domyślnego kokpitu mojego samochodu, takie jak spalanie, średnia prędkość na trasie, pokonany dystans od rozruchu, obciążenie silnika i wiele innych. Zdecydowałem się, że projekt opiszę niechronologicznie, a idąc "po kablu" od początku zasilania (najmniej skrupulatnie), aż do mózgu operacji i jego wyjść. Zaznaczam że to mój pierwszy projekt wykraczający poza Arduino UNO i liczę się z tym że moja praca może zostać odebrana jako fuszerka  ale no cóż każdy kiedyś zaczynał i z chęcią usłyszę jakieś wskazówki, zaczynajmy!


## 📏Schemat

Zdecydowałem się, że dane będę czytał bezpośrednio ze złącza diagnostycznego OBDII za pomocą taniego adaptera ELM327 z interfejsem bluetooth, a mózgiem operacji zostanie płytka ESP32 Devkit v1, która odpowiednio zaprogramowana może pośrednio łącząc się z wcześniej wspomnianym adapterem, zdobywać dane z ECU a nawet usuwać błędy. Dodatkowo zależało mi na zasilaniu po zapłonie dla tego projektu i kamerki, bo zapalniczka podaje prąd cały czas. Po namyśle zdecydowałem się na taki (tutaj rysunkowy co prawda) schemat:




## ⚡Zasilanie

Chciałem, aby układ był aktywny tylko gdy tego potrzebuję bez zbędncyh interakcji z nim, dlatego zdecydowałem się na wykorzystanie wyjścia z przekaźnika aktywowanego po przekręceniu kluczyka - szyny 75x i masy. Z pomocą zaciskarki, konektorów oczkowych i odpowiednich pod względem przekroju przewodów (+puszki WD40 by w ogóle śruby odkręcić po 23 latach  ) udało się uzyskać napięcie ok. 12V. Na linii dodatniej zastosowałem bezpiecznik szklany na 2A. Kolejnym etapem było wykorzystanie przetwornicy step-down LM2596, która bezpiecznie bez radiatora może wyciągnąć na wyjściu 2A , co w moim przypadku jest i tak za duże. Przetwornicę wyregulowałem i dopieściłem obudową wydrukowaną na nowo zakupionej drukarce 3D Bambu Lab A1 Mini i przylutowałem przewody IN/OUT. Tym sposobem udało się uzyskać 5V co oznacza koniec etapu zasilania 





## 🔌Rozdzielenie prądu 

Wiadomą rzeczą jest, że prąd nie jest potrzebny tylko do zasilania płytki, ale wyświetlacza, joysticka i w bonusie również kamerki dlatego na bazie płytki uniwersalnej do THT i złączy AVR raster zlutowałem taki moduł, gdzie jedna szyna to + druga to -, może niezbyt estetyczne rozwiązanie ze wzlędu na to, że kable będą musiały być rozdzielone na przewody o wiele wcześniej niż gdyby każde złącze śrubowe miało swój + i - ale za to prostsze i bezpieczniejsze moim zdaniem. Na ten moduł zaprojektowałem w Autodesk Inventor obudowę i wydrukowałem.



 3) 

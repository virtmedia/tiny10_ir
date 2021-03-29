/*

Biblioteka dekodowania sygna³u RC-5 np. z pilota podczerwieni.
Wersja:	1.0

Biblioteka powsta³a do celów nauki sposobu dekodowania sygna³u RC-5 
i nie jest optymalizowana pod k¹tem objêtoœci kodu.

Mikrokontrolery AVR.
Testowana na ATmega8 z kompilatorem AVR-GCC 4.3.3

Plik:	dd_rc5.c

Data: 	2013.07.30 
Autor: 	Dondu
www:	http://mikrokontrolery.blogspot.com/2011/03/RC5-IR-podczerwien-odbior-danych-przyklad-AVR-ATmega8.html

*/

#include <avr/io.h>
#include <avr/interrupt.h>

#include "dd_rc5.h"

volatile unsigned char dd_rc5_dane_odebrane;
volatile unsigned char dd_rc5_status;


//-----------------------------------------------------------------------------


void dd_rc5_ini(void){
	
	//Funkcja inicjuj¹ca Timer oraz przerwania	oraz LED testowy

	//ustaw pin diody LED sygnalizuj¹cej przerwania
	#ifdef _DD_RC5_WLACZ_DIODE_PRZERWAN_
		DD_RC5_LED_PRZ_DDR	 |=   (1<<DD_RC5_LED_PRZ_PIN);
		DD_RC5_LED_PRZ_PORT	 &=  ~(1<<DD_RC5_LED_PRZ_PIN);
	#endif


	//Ustaw pin przerwania jako wejœcie bez w³¹czonego rezystora pull-up
    DD_RC5_PRZERWANIE_INT_DDR  &= ~(1<<DD_RC5_PRZERWANIE_INT_PIN); 
    DD_RC5_PRZERWANIE_INT_PORT &= ~(1<<DD_RC5_PRZERWANIE_INT_PIN);


	//ustaw wykrywanie zbocza opadaj¹cego na pinie INT0 (PD2)
	//zgodnie ze standardem RC5 zbocze opadaj¹ce bêdzie pierwszym zboczem
	//pierwszego bitu startu
    EICRA |= (1<<ISC01);

	//W³¹cz przerwania z pinu INT0
    DD_RC5_WLACZ_DEKODOWANIE;

    //Ustawienia timera
    TCCR0B = DD_RC5_TIMER_PRESKALER_BITY;

	//Je¿eli wczeœniej u¿ywa³eœ tego timer0, to odkomentuj poni¿sze linie
	GTCCR |= (1<<PSR);	//resetuj preskaler timer0
	TCNT0 = 0x00;			//zeruj timer0
    TIFR0 |= (1<<TOV0); 	//zgaœ flagê przerwania timer0
}


//-----------------------------------------------------------------------------

//funkcja zeruj¹ca timer oraz flagê jego przepe³nienia
inline void dd_rc5_zeruj_timer_i_flage_przepelnienia(void){
	GTCCR |= (1<<PSR);	//resetuj preskaler timera
	TCNT0 = 0x00; 			//zeruj timer
	TIFR0 |= (1<<TOV0);		//zgaœ flagê przepe³nienia timera
}

//-----------------------------------------------------------------------------

inline void dd_rc5_ustaw_zbocze_opadajace(void){
	EICRA &= ~(1<<ISC00);	//ustaw wykrywanie zbocza opadaj¹cego
}

//-----------------------------------------------------------------------------

inline void dd_rc5_ustaw_zbocze_narastajace(void){
	EICRA |= (1<<ISC00);	//ustaw wykrywanie zbocza narastaj¹cego
}

//-----------------------------------------------------------------------------

inline void dd_rc5_zmien_zbocze_na_przeciwne(void){
	//ustaw wykrywanie zbocza przeciwnego do aktualnie ustawionego
	EICRA ^= (1<<ISC00);	
}


//-----------------------------------------------------------------------------

ISR(DD_RC5_PRZERWANIE_WEKTOR)
{
	EICRA |= (1<<ISC01);
	//Obs³uga przerwania wykrytego zbocza sygna³u z czujnika podczerwieni

	//W zale¿noœci od ustawieñ w danym momencie rejestru EICRA 
	//przerwanie wykonywane jest przy wykryciu narastaj¹cego lub opadaj¹cego 
	//zbocza sygna³u z czujnika podczerwieni (bity ISC00 i ISC01)
	//Do wykrycia kierunku zbocza wystarczy bit ISC00.

	//UWAGA dot. flagi TOV0
	//W poni¿szych porównaniach czasu trwania impulsu sprawdzamy,
	//tak¿e flagê przepe³nienia timera (flaga TOV0), poniewa¿ samo 
	//porównanie wartoœci timera jest niewystarczaj¹ce,
	//gdy¿ móg³ on zostaæ przepe³niony.
	
	//zmienne pomocnicze statyczne dostêpne tylko w tej funkcji
	static unsigned char 	dd_rc5_numer_bitu = 1;
	static unsigned char 	dd_rc5_polbit_licznik = 0;
	static unsigned int 	dd_rc5_dane_temp = 0;
EICRA |= (1<<ISC01);
	//w³¹cz diodê sygnalizuj¹c¹ przerwanie
	#ifdef _DD_RC5_WLACZ_DIODE_PRZERWAN_
		DD_RC5_LED_PRZ_ON;
	#endif



  //Je¿eli program g³ówny lub inny fragment programu, nie odczyta³
  //odebranch wczeœniej danych, to wychodzimy z przerwania 
  //i nie dekodujemy w³aœnie nadawanej przez pilota ramki.
  //Jest to zabezpieczenie, przed nadpisaniem poprzednio odebranych,
  //a jeszcze nie u¿ytych danych z pilota.
  if(!(dd_rc5_status & DD_RC5_STATUS_DANE_GOTOWE_DO_ODCZYTU)){
	

	//Na samym pocz¹tku zmieniamy wykrywanie zbocza przeciwnego do 
	//poprzedniego. Robimy to po to, by nie umknê³o nam ¿adne przerwanie,
	//które mo¿e wyst¹piæ w trakcie wykonywania niniejszej funkcji przerwania.
	//Utrata wiedzy o wyst¹pieniu przerwania mog³aby spowodowaæ b³êdne
	//dekodowanie sygna³u.
	//--- UWAGA!!! ---
	//Poniewa¿ w dalszej czêœci programu musimy znaæ kierunek zbocza, które 
	//wywo³a³o przerwanie, st¹d mo¿esz odnieœæ mylne wra¿enie, ¿e warunki 
	//if() oraz komentarze ich dotycz¹ce, nie s¹ spójne. Gdy bêdziesz je 
	//analizowa³ pamiêtaj, ¿e ju¿ tutaj  zmieniamy zbocze na przeciwne!!!
	dd_rc5_zmien_zbocze_na_przeciwne();

	
	//W zale¿noœci, który bit jest aktualnie dekodowany
	switch(dd_rc5_numer_bitu){

		//--- bit startowy nr 1 ------------------------------------
	
		case 1:	

			//Wykryto pierwsze zbocze opadaj¹ce.
			//Sprawdzamy, czy przerwa w sygnale by³a wystarczaj¹co d³uga,
			//by stwierdziæ, ¿e to pierwsze zbocze opadaj¹ce pierwszego
			//bitu startu. Tylko w takim przypadku zaczniemy dekodowaæ sygna³. 
			//Je¿eli przerwa by³a zbyt krótka, nale¿y zaczekaæ 
			//na nastêpn¹ ramkê danych.
			//
			//UWAGA!! Tutaj stosujemy prost¹ zasadê dla warunku czasu cziszy
			//przed pierwszym bitem startu polegaj¹c¹ na przyjêciu, ¿e czas
			//ciszy musi byæ d³u¿szy ni¿ czas przepe³nienia timera. Jest to
			//œwiadome uproszczenie algorytmu - szczegó³y w linku.
			if(
				(TIFR0 & (1<<TOV0))	//je¿eli wyst¹pi³o przepe³nienie
				//mo¿na tak¿e inaczej:
				//lub licznik odliczy³ czas wymaganego minimum
				//przyjmujemy czas równy 1.5 bitu
				//|| (TCNT0 > (DD_RC5_OKRES_BITU_MAX + DD_RC5_OKRES_POLOWY_BITU_MAX))
				//|| (TCNT0 > 250)
				//albo napisz w³asny inny warunek
			){


				//Przerwa miêdzy kolejnymi ramkami by³a wystarczaj¹ca, 
				//by mieæ pewnoœæ, ¿e rozpoczynamy odbiór nowej ramki.

				dd_rc5_zeruj_timer_i_flage_przepelnienia();

				//nastêpny bit bêdzie bitem nr 2
				dd_rc5_numer_bitu=2;

				//wyzeruj pomocnicz¹ zmienn¹ odebranych danych
				dd_rc5_dane_temp = 0;
				dd_rc5_polbit_licznik = 0;


				//koñczymy czekaj¹c na zbocze narastaj¹ce drugiego bitu startu

			}else{
				//Zbyt krótka przerwa w sygnale pomiêdzy ramkami
				//dlatego tê ramkê danych musimy przeczekaæ.

				//Tutaj nie trzeba nic wykonywaæ, poniewa¿ bit ma numer 1 
				//co oznacza, ¿e warunek na koñczu funkcji przerwania
				//ustawi stan pocz¹tkowy.

			}
			break;


		//--- bit startowy nr 2 ------------------------------------

		case 2:

			//sprawdzamy, czy czas po³owy bitu jest zgodny z parametrami
			//oraz flagê przepe³nienia zgodnie z uwag¹ na pocz¹tku funkcji
			if(
				(TIFR0 & (1<<TOV0)) //je¿eli wyst¹pi³o przepe³nienie timera
				|| (TCNT0 < DD_RC5_OKRES_POLOWY_BITU_MIN) //lub pó³bit za krótki
				|| (TCNT0 > DD_RC5_OKRES_POLOWY_BITU_MAX) //lub pó³bit za d³ugi
			){
				
				//Wykryto b³¹d w sygnale poniewa¿ czas jest inny ni¿ 
				//dopuszczalny. Przerywamy dekodowanie tej ramki
				//sygna³u i rozpoczynamy od nowej ramki
				dd_rc5_numer_bitu = 1;

			}else{ 

				//Czas jest z zakresu pó³bitów		

				//Które zbocze wywo³a³o przerwanie?
				if(EICRA & (1<<ISC00)){

					//Przerwanie wywo³a³o zbocze opadaj¹ce bitu startowego nr 2
					//co oznacza, ¿e drugi bit startu odebrany prawid³owo

					dd_rc5_zeruj_timer_i_flage_przepelnienia();

					//zwiêksz licznik bitów
					dd_rc5_numer_bitu++;


				}else{

					//wykryto pierwsze zbocze narastaj¹ce bitu startowego nr 2

					dd_rc5_zeruj_timer_i_flage_przepelnienia();

				}

			}
			break;


		//--- pozosta³e bity --------------------------------------

		default:

			//Tutaj odbieramy pozosta³e bity ramki			

			//W zale¿noœci jaki czas up³yn¹³ od ostatniego zbocza (przerwania)
			//Czy czas wykracza poza brzegowe parametry (min i max)
			if(
				(TIFR0 & (1<<TOV0)) //przepe³nienie timera to b³¹d
				|| (TCNT0 < DD_RC5_OKRES_POLOWY_BITU_MIN)
				|| (TCNT0 > DD_RC5_OKRES_BITU_MAX)

			){

				//Wykryto b³¹d w sygnale poniewa¿ czas nie mieœci siê 
				//w za³o¿onych progach bitu ani pó³bitu. Przerywamy wiêc
				//dekodowanie tej ramki sygna³u i rozpoczynamy 
				//oczekiwanie na pocz¹tek kolejnej ramki

				//przygotuj siê do odbioru nowej ramki
				dd_rc5_numer_bitu = 1;
				break;  //break, by nie sprawdza³ poni¿szego warunku





			//Czy min¹³ czas pó³bitu?
			}else if( 
				(TCNT0 <= DD_RC5_OKRES_POLOWY_BITU_MAX)
				//warunku MIN nie musimy sprawdzaæ, poniewa¿
				//zosta³ ju¿ sprawdzony na pocz¹tku niniejszego if()
				//&& (TCNT0 >= DD_RC5_OKRES_POLOWY_BITU_MIN) 	
				 
			){

				//Up³yn¹³ czas równy po³owie bitu

				//sprawdzamy, czy to druga po³owa bitu
				if(dd_rc5_polbit_licznik){
						     
					//Tak to druga po³ówka aktualnie dekodowanego bitu
					//i jesteœmy aktualnie w po³owie czasu odbieranego
					//bitu. Jest to moment, w którym ustalamy wartoœæ
					//odebranego bitu na podstawie kierunku zbocza.

					//Przesuñ dane o jeden bit w lewo, by zrobiæ miejsce
					//na odebrany bit
					dd_rc5_dane_temp <<= 1;

					//Je¿eli aktualnie ustawione jest zbocze narastaj¹ce 
					//to znaczy, ¿e przerwanie zosta³o wywo³ane przez zbocze 
					//opadaj¹ce, a to oznacza, ¿e odebraliœmy jedynkê logiczn¹,
					//któr¹ nale¿y dodaæ na najmniej znacz¹cej pozycji rejestru
					//odbiorczego. Czytaj uwagê na pocz¹tku tej funkcji.
					if(EICRA & (1<<ISC00)) dd_rc5_dane_temp |= 1;

					//wyzeruj licznik pó³bitów
					dd_rc5_polbit_licznik = 0;

					//zwiêksz licznik bitów
					dd_rc5_numer_bitu++;

				}else{

					//To pierwsza po³owa dekodowanego bitu, czyli jesteœmy 
					//aktualnie na pocz¹tku czasu przesy³anego bitu.

					//Ustawiamy licznik pó³bitów
					dd_rc5_polbit_licznik = 1;

				}

				dd_rc5_zeruj_timer_i_flage_przepelnienia();


			
			}else{

				//Up³yn¹³ czas ca³ego bitu i jesteœmy aktualnie w po³owie
				//odbieranego bitu. Jest to moment, w którym ustalamy wartoœæ
				//odebranego bitu na podstawie kierunku zbocza.

				//Przesuñ rejestr odbiorczy o jeden bit w lewo by zrobiæ 
				//miejsce na odebrany bit
				dd_rc5_dane_temp <<= 1;

				//Je¿eli aktualnie ustawione jest zbocze narastaj¹ce 
				//to znaczy, ¿e przerwanie zosta³o wywo³ane przez zbocze 
				//opadaj¹ce, a to oznacza, ¿e odebraliœmy jedynkê logiczn¹,
				//któr¹ nale¿y dodaæ na najmniej znacz¹cej pozycji rejestru
				//odbiorczego. Czytaj uwagê na pocz¹tku tej funkcji.
				if(EICRA & (1<<ISC00)) dd_rc5_dane_temp |= 1;

				dd_rc5_zeruj_timer_i_flage_przepelnienia();

				//zwiêksz licznik bitów
				dd_rc5_numer_bitu++;

			}


			//czy to ju¿ ostatni bit?
			if(dd_rc5_numer_bitu > 14){

				//Tak, odebrano ostatni bit

				//Zapisz dane do zmiennych globalnych

				//1-Dane s¹ na szeœciu najm³odszych bitach
				dd_rc5_dane_odebrane = dd_rc5_dane_temp & 0b111111;

				//2-Adres urz¹dzenia i bit toggle 
				//przesuñ w prawo o 6 pozycji pozbywaj¹c siê bitów danych
				dd_rc5_dane_temp >>= 6; 
				//przepisz bity do rejestru statusu i dodaj bit gotowoœci
				//danych do odczytu
				dd_rc5_status = (dd_rc5_dane_temp & 0b111111) 
								| DD_RC5_STATUS_DANE_GOTOWE_DO_ODCZYTU;

				//przygotuj siê do odbioru nowej ramki
				dd_rc5_numer_bitu = 1;

				//Wy³¹czamy dekodowanie (blokada przerwania INT), do momentu
				//gdy program g³ówny ponownie go w³¹czy
				DD_RC5_WYLACZ_DEKODOWANIE;
									
				//koniec odbioru ramki ... uff nareszcie :-)
			}
			break;
	}
  }


  //Je¿eli na koñcu funkcji przerwania numer bitu ma wartoœæ 1
  //to oznacza, ¿e zarz¹dano przerwania dekodowania z powodu b³êdu
  if(dd_rc5_numer_bitu==1){
  	//ustawiamy stan pocz¹tkowy 
	dd_rc5_zeruj_timer_i_flage_przepelnienia();
	dd_rc5_ustaw_zbocze_opadajace();
	dd_rc5_polbit_licznik = 0;
  }


  //wy³¹cz diodê sygnalizuj¹c¹ przerwanie
  #ifdef _DD_RC5_WLACZ_DIODE_PRZERWAN_
	DD_RC5_LED_PRZ_OFF;
  #endif

}


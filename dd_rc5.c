/*

Biblioteka dekodowania sygna�u RC-5 np. z pilota podczerwieni.
Wersja:	1.0

Biblioteka powsta�a do cel�w nauki sposobu dekodowania sygna�u RC-5 
i nie jest optymalizowana pod k�tem obj�to�ci kodu.

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
	
	//Funkcja inicjuj�ca Timer oraz przerwania	oraz LED testowy

	//ustaw pin diody LED sygnalizuj�cej przerwania
	#ifdef _DD_RC5_WLACZ_DIODE_PRZERWAN_
		DD_RC5_LED_PRZ_DDR	 |=   (1<<DD_RC5_LED_PRZ_PIN);
		DD_RC5_LED_PRZ_PORT	 &=  ~(1<<DD_RC5_LED_PRZ_PIN);
	#endif


	//Ustaw pin przerwania jako wej�cie bez w��czonego rezystora pull-up
    DD_RC5_PRZERWANIE_INT_DDR  &= ~(1<<DD_RC5_PRZERWANIE_INT_PIN); 
    DD_RC5_PRZERWANIE_INT_PORT &= ~(1<<DD_RC5_PRZERWANIE_INT_PIN);


	//ustaw wykrywanie zbocza opadaj�cego na pinie INT0 (PD2)
	//zgodnie ze standardem RC5 zbocze opadaj�ce b�dzie pierwszym zboczem
	//pierwszego bitu startu
    EICRA |= (1<<ISC01);

	//W��cz przerwania z pinu INT0
    DD_RC5_WLACZ_DEKODOWANIE;

    //Ustawienia timera
    TCCR0B = DD_RC5_TIMER_PRESKALER_BITY;

	//Je�eli wcze�niej u�ywa�e� tego timer0, to odkomentuj poni�sze linie
	GTCCR |= (1<<PSR);	//resetuj preskaler timer0
	TCNT0 = 0x00;			//zeruj timer0
    TIFR0 |= (1<<TOV0); 	//zga� flag� przerwania timer0
}


//-----------------------------------------------------------------------------

//funkcja zeruj�ca timer oraz flag� jego przepe�nienia
inline void dd_rc5_zeruj_timer_i_flage_przepelnienia(void){
	GTCCR |= (1<<PSR);	//resetuj preskaler timera
	TCNT0 = 0x00; 			//zeruj timer
	TIFR0 |= (1<<TOV0);		//zga� flag� przepe�nienia timera
}

//-----------------------------------------------------------------------------

inline void dd_rc5_ustaw_zbocze_opadajace(void){
	EICRA &= ~(1<<ISC00);	//ustaw wykrywanie zbocza opadaj�cego
}

//-----------------------------------------------------------------------------

inline void dd_rc5_ustaw_zbocze_narastajace(void){
	EICRA |= (1<<ISC00);	//ustaw wykrywanie zbocza narastaj�cego
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
	//Obs�uga przerwania wykrytego zbocza sygna�u z czujnika podczerwieni

	//W zale�no�ci od ustawie� w danym momencie rejestru EICRA 
	//przerwanie wykonywane jest przy wykryciu narastaj�cego lub opadaj�cego 
	//zbocza sygna�u z czujnika podczerwieni (bity ISC00 i ISC01)
	//Do wykrycia kierunku zbocza wystarczy bit ISC00.

	//UWAGA dot. flagi TOV0
	//W poni�szych por�wnaniach czasu trwania impulsu sprawdzamy,
	//tak�e flag� przepe�nienia timera (flaga TOV0), poniewa� samo 
	//por�wnanie warto�ci timera jest niewystarczaj�ce,
	//gdy� m�g� on zosta� przepe�niony.
	
	//zmienne pomocnicze statyczne dost�pne tylko w tej funkcji
	static unsigned char 	dd_rc5_numer_bitu = 1;
	static unsigned char 	dd_rc5_polbit_licznik = 0;
	static unsigned int 	dd_rc5_dane_temp = 0;
EICRA |= (1<<ISC01);
	//w��cz diod� sygnalizuj�c� przerwanie
	#ifdef _DD_RC5_WLACZ_DIODE_PRZERWAN_
		DD_RC5_LED_PRZ_ON;
	#endif



  //Je�eli program g��wny lub inny fragment programu, nie odczyta�
  //odebranch wcze�niej danych, to wychodzimy z przerwania 
  //i nie dekodujemy w�a�nie nadawanej przez pilota ramki.
  //Jest to zabezpieczenie, przed nadpisaniem poprzednio odebranych,
  //a jeszcze nie u�ytych danych z pilota.
  if(!(dd_rc5_status & DD_RC5_STATUS_DANE_GOTOWE_DO_ODCZYTU)){
	

	//Na samym pocz�tku zmieniamy wykrywanie zbocza przeciwnego do 
	//poprzedniego. Robimy to po to, by nie umkn�o nam �adne przerwanie,
	//kt�re mo�e wyst�pi� w trakcie wykonywania niniejszej funkcji przerwania.
	//Utrata wiedzy o wyst�pieniu przerwania mog�aby spowodowa� b��dne
	//dekodowanie sygna�u.
	//--- UWAGA!!! ---
	//Poniewa� w dalszej cz�ci programu musimy zna� kierunek zbocza, kt�re 
	//wywo�a�o przerwanie, st�d mo�esz odnie�� mylne wra�enie, �e warunki 
	//if() oraz komentarze ich dotycz�ce, nie s� sp�jne. Gdy b�dziesz je 
	//analizowa� pami�taj, �e ju� tutaj  zmieniamy zbocze na przeciwne!!!
	dd_rc5_zmien_zbocze_na_przeciwne();

	
	//W zale�no�ci, kt�ry bit jest aktualnie dekodowany
	switch(dd_rc5_numer_bitu){

		//--- bit startowy nr 1 ------------------------------------
	
		case 1:	

			//Wykryto pierwsze zbocze opadaj�ce.
			//Sprawdzamy, czy przerwa w sygnale by�a wystarczaj�co d�uga,
			//by stwierdzi�, �e to pierwsze zbocze opadaj�ce pierwszego
			//bitu startu. Tylko w takim przypadku zaczniemy dekodowa� sygna�. 
			//Je�eli przerwa by�a zbyt kr�tka, nale�y zaczeka� 
			//na nast�pn� ramk� danych.
			//
			//UWAGA!! Tutaj stosujemy prost� zasad� dla warunku czasu cziszy
			//przed pierwszym bitem startu polegaj�c� na przyj�ciu, �e czas
			//ciszy musi by� d�u�szy ni� czas przepe�nienia timera. Jest to
			//�wiadome uproszczenie algorytmu - szczeg�y w linku.
			if(
				(TIFR0 & (1<<TOV0))	//je�eli wyst�pi�o przepe�nienie
				//mo�na tak�e inaczej:
				//lub licznik odliczy� czas wymaganego minimum
				//przyjmujemy czas r�wny 1.5 bitu
				//|| (TCNT0 > (DD_RC5_OKRES_BITU_MAX + DD_RC5_OKRES_POLOWY_BITU_MAX))
				//|| (TCNT0 > 250)
				//albo napisz w�asny inny warunek
			){


				//Przerwa mi�dzy kolejnymi ramkami by�a wystarczaj�ca, 
				//by mie� pewno��, �e rozpoczynamy odbi�r nowej ramki.

				dd_rc5_zeruj_timer_i_flage_przepelnienia();

				//nast�pny bit b�dzie bitem nr 2
				dd_rc5_numer_bitu=2;

				//wyzeruj pomocnicz� zmienn� odebranych danych
				dd_rc5_dane_temp = 0;
				dd_rc5_polbit_licznik = 0;


				//ko�czymy czekaj�c na zbocze narastaj�ce drugiego bitu startu

			}else{
				//Zbyt kr�tka przerwa w sygnale pomi�dzy ramkami
				//dlatego t� ramk� danych musimy przeczeka�.

				//Tutaj nie trzeba nic wykonywa�, poniewa� bit ma numer 1 
				//co oznacza, �e warunek na ko�czu funkcji przerwania
				//ustawi stan pocz�tkowy.

			}
			break;


		//--- bit startowy nr 2 ------------------------------------

		case 2:

			//sprawdzamy, czy czas po�owy bitu jest zgodny z parametrami
			//oraz flag� przepe�nienia zgodnie z uwag� na pocz�tku funkcji
			if(
				(TIFR0 & (1<<TOV0)) //je�eli wyst�pi�o przepe�nienie timera
				|| (TCNT0 < DD_RC5_OKRES_POLOWY_BITU_MIN) //lub p�bit za kr�tki
				|| (TCNT0 > DD_RC5_OKRES_POLOWY_BITU_MAX) //lub p�bit za d�ugi
			){
				
				//Wykryto b��d w sygnale poniewa� czas jest inny ni� 
				//dopuszczalny. Przerywamy dekodowanie tej ramki
				//sygna�u i rozpoczynamy od nowej ramki
				dd_rc5_numer_bitu = 1;

			}else{ 

				//Czas jest z zakresu p�bit�w		

				//Kt�re zbocze wywo�a�o przerwanie?
				if(EICRA & (1<<ISC00)){

					//Przerwanie wywo�a�o zbocze opadaj�ce bitu startowego nr 2
					//co oznacza, �e drugi bit startu odebrany prawid�owo

					dd_rc5_zeruj_timer_i_flage_przepelnienia();

					//zwi�ksz licznik bit�w
					dd_rc5_numer_bitu++;


				}else{

					//wykryto pierwsze zbocze narastaj�ce bitu startowego nr 2

					dd_rc5_zeruj_timer_i_flage_przepelnienia();

				}

			}
			break;


		//--- pozosta�e bity --------------------------------------

		default:

			//Tutaj odbieramy pozosta�e bity ramki			

			//W zale�no�ci jaki czas up�yn�� od ostatniego zbocza (przerwania)
			//Czy czas wykracza poza brzegowe parametry (min i max)
			if(
				(TIFR0 & (1<<TOV0)) //przepe�nienie timera to b��d
				|| (TCNT0 < DD_RC5_OKRES_POLOWY_BITU_MIN)
				|| (TCNT0 > DD_RC5_OKRES_BITU_MAX)

			){

				//Wykryto b��d w sygnale poniewa� czas nie mie�ci si� 
				//w za�o�onych progach bitu ani p�bitu. Przerywamy wi�c
				//dekodowanie tej ramki sygna�u i rozpoczynamy 
				//oczekiwanie na pocz�tek kolejnej ramki

				//przygotuj si� do odbioru nowej ramki
				dd_rc5_numer_bitu = 1;
				break;  //break, by nie sprawdza� poni�szego warunku





			//Czy min�� czas p�bitu?
			}else if( 
				(TCNT0 <= DD_RC5_OKRES_POLOWY_BITU_MAX)
				//warunku MIN nie musimy sprawdza�, poniewa�
				//zosta� ju� sprawdzony na pocz�tku niniejszego if()
				//&& (TCNT0 >= DD_RC5_OKRES_POLOWY_BITU_MIN) 	
				 
			){

				//Up�yn�� czas r�wny po�owie bitu

				//sprawdzamy, czy to druga po�owa bitu
				if(dd_rc5_polbit_licznik){
						     
					//Tak to druga po��wka aktualnie dekodowanego bitu
					//i jeste�my aktualnie w po�owie czasu odbieranego
					//bitu. Jest to moment, w kt�rym ustalamy warto��
					//odebranego bitu na podstawie kierunku zbocza.

					//Przesu� dane o jeden bit w lewo, by zrobi� miejsce
					//na odebrany bit
					dd_rc5_dane_temp <<= 1;

					//Je�eli aktualnie ustawione jest zbocze narastaj�ce 
					//to znaczy, �e przerwanie zosta�o wywo�ane przez zbocze 
					//opadaj�ce, a to oznacza, �e odebrali�my jedynk� logiczn�,
					//kt�r� nale�y doda� na najmniej znacz�cej pozycji rejestru
					//odbiorczego. Czytaj uwag� na pocz�tku tej funkcji.
					if(EICRA & (1<<ISC00)) dd_rc5_dane_temp |= 1;

					//wyzeruj licznik p�bit�w
					dd_rc5_polbit_licznik = 0;

					//zwi�ksz licznik bit�w
					dd_rc5_numer_bitu++;

				}else{

					//To pierwsza po�owa dekodowanego bitu, czyli jeste�my 
					//aktualnie na pocz�tku czasu przesy�anego bitu.

					//Ustawiamy licznik p�bit�w
					dd_rc5_polbit_licznik = 1;

				}

				dd_rc5_zeruj_timer_i_flage_przepelnienia();


			
			}else{

				//Up�yn�� czas ca�ego bitu i jeste�my aktualnie w po�owie
				//odbieranego bitu. Jest to moment, w kt�rym ustalamy warto��
				//odebranego bitu na podstawie kierunku zbocza.

				//Przesu� rejestr odbiorczy o jeden bit w lewo by zrobi� 
				//miejsce na odebrany bit
				dd_rc5_dane_temp <<= 1;

				//Je�eli aktualnie ustawione jest zbocze narastaj�ce 
				//to znaczy, �e przerwanie zosta�o wywo�ane przez zbocze 
				//opadaj�ce, a to oznacza, �e odebrali�my jedynk� logiczn�,
				//kt�r� nale�y doda� na najmniej znacz�cej pozycji rejestru
				//odbiorczego. Czytaj uwag� na pocz�tku tej funkcji.
				if(EICRA & (1<<ISC00)) dd_rc5_dane_temp |= 1;

				dd_rc5_zeruj_timer_i_flage_przepelnienia();

				//zwi�ksz licznik bit�w
				dd_rc5_numer_bitu++;

			}


			//czy to ju� ostatni bit?
			if(dd_rc5_numer_bitu > 14){

				//Tak, odebrano ostatni bit

				//Zapisz dane do zmiennych globalnych

				//1-Dane s� na sze�ciu najm�odszych bitach
				dd_rc5_dane_odebrane = dd_rc5_dane_temp & 0b111111;

				//2-Adres urz�dzenia i bit toggle 
				//przesu� w prawo o 6 pozycji pozbywaj�c si� bit�w danych
				dd_rc5_dane_temp >>= 6; 
				//przepisz bity do rejestru statusu i dodaj bit gotowo�ci
				//danych do odczytu
				dd_rc5_status = (dd_rc5_dane_temp & 0b111111) 
								| DD_RC5_STATUS_DANE_GOTOWE_DO_ODCZYTU;

				//przygotuj si� do odbioru nowej ramki
				dd_rc5_numer_bitu = 1;

				//Wy��czamy dekodowanie (blokada przerwania INT), do momentu
				//gdy program g��wny ponownie go w��czy
				DD_RC5_WYLACZ_DEKODOWANIE;
									
				//koniec odbioru ramki ... uff nareszcie :-)
			}
			break;
	}
  }


  //Je�eli na ko�cu funkcji przerwania numer bitu ma warto�� 1
  //to oznacza, �e zarz�dano przerwania dekodowania z powodu b��du
  if(dd_rc5_numer_bitu==1){
  	//ustawiamy stan pocz�tkowy 
	dd_rc5_zeruj_timer_i_flage_przepelnienia();
	dd_rc5_ustaw_zbocze_opadajace();
	dd_rc5_polbit_licznik = 0;
  }


  //wy��cz diod� sygnalizuj�c� przerwanie
  #ifdef _DD_RC5_WLACZ_DIODE_PRZERWAN_
	DD_RC5_LED_PRZ_OFF;
  #endif

}


/*

Biblioteka dekodowania sygna�u RC-5 np. z pilota podczerwieni.
Wersja:	1.0

Biblioteka powsta�a do cel�w nauki sposobu dekodowania sygna�u RC-5 
i nie jest optymalizowana pod k�tem obj�to�ci kodu.

Mikrokontrolery AVR.
Testowana na ATmega8 z kompilatorem AVR-GCC 4.3.3

Plik:	dd_rc5.h

Data: 	2013.07.30 
Autor: 	Dondu
www:	http://mikrokontrolery.blogspot.com/2011/03/RC5-IR-podczerwien-odbior-danych-przyklad-AVR-ATmega8.html


*/


#ifndef _DD_RC5_H_
#define _DD_RC5_H_ 1


	/*=== STROJENIE ODBIORNIKA =============================
	Je�eli pilot nie odbiera prawid�owo danych z pilota RC-5 powiniene�
	dobra� PRESKALER oraz TOLERANCJ� - oba parametry znajdziesz poni�ej. 
	Aby mie� pewno��, �e Tw�j uk�ad dzia�a i wykrywa sygna� RC-5 mo�esz 
	doda� diod� LED. Szczeg�y w linku podanym wy�ej lub opisach poni�ej */
	

	/*=== LED PRZERWANIA ================================
	LED testowy do pokazywania momentu rozpocz�cia i zako�czenia przerwania,
	co pozwala �ledzi� prac� programu, gdy odbiera dane w szczeg�lno�ci 
	z pilot�w innych standard�w ni� RC-5. Szczeg�y w linku podanym wy�ej */
	#define DD_RC5_LED_PRZ_DDR 		DDRB
	#define DD_RC5_LED_PRZ_PORT		PORTB
	#define DD_RC5_LED_PRZ_PIN 		PORTB0
	//UWAGA!!! 
	//Aby wy��czy� diod� LED na sta�e zakomentuj poni�sz� definicj� */
    //#define _DD_RC5_WLACZ_DIODE_PRZERWAN_	1

	
	/*=== PRESKALER timera ==================================

	UWAGA!!! 
	Zmieniaj�c preskaler jest prawdopodobne, �e zmieniasz tak�e cz�stotliwo��
	zegara mikrokontrolera za pomoc� fusebit�w. Je�eli tak, to nie zapomnij 
	zmieni� F_CPU w opcjach projektu!!!
	
	Tutaj podaj warto�� preskalera wybieraj�c 8, 64, 256 lub 1024 */
	#define DD_RC5_TIMER_PRESKALER 		8


	/*=== TOLERANCJA SYGNA�U =================================
	Parametry tolerancji (mikrosekundy) sprawdzania czas�w odbieranego sygna�u.
	Je�eli masz k�opoty z odbiorem w szczeg�lno�ci, gdy nie ka�da komenda jest
	odczytywana mo�esz do�wiadczalnie regulowa� parametrem tolerancji 
	ustawiaj�c w�asn� warto�� DD_RC5_TOLERANCJA_US.	Im ten parametr jest 
	mniejszy tym mniejsza tolerancja na b��dy.	
	*/
	#define DD_RC5_TOLERANCJA_US 350	//Sugeruj� pr�bowa� od 150 do 350
										//ale mo�na tak�e inne warto�ci
										//... tylko nie przesadzaj :-)  


	/*=== PRZERWANIE =======================================
	Wybierz przerwanie, kt�re u�ywasz oraz ustaw odpowiednie 
	DDR i symbol pinu */
	#define DD_RC5_PRZERWANIE_INT			INT0
	#define DD_RC5_PRZERWANIE_WEKTOR		INT0_vect
	#define DD_RC5_PRZERWANIE_INT_PIN		PORTB2		
	#define DD_RC5_PRZERWANIE_INT_DDR		DDRB	
	#define DD_RC5_PRZERWANIE_INT_PORT		PORTB






	/*** Poni�ej nic nie zmieniaj!!! ******************/


	//--- DIODA LED SYGNALIZACJI PRZERWA� ------------------------------
	#if _DD_RC5_WLACZ_DIODE_PRZERWAN_
		#define DD_RC5_LED_PRZ_ON	DD_RC5_LED_PRZ_PORT |=  (1<<DD_RC5_LED_PRZ_PIN)
		#define DD_RC5_LED_PRZ_OFF	DD_RC5_LED_PRZ_PORT &= ~(1<<DD_RC5_LED_PRZ_PIN)
	#endif



	//--- Przerwanie ----------------------------------------

	#define DD_RC5_WLACZ_DEKODOWANIE 	EIMSK  |= (1<<DD_RC5_PRZERWANIE_INT)
	#define DD_RC5_WYLACZ_DEKODOWANIE 	EIMSK  &= ~(1<<DD_RC5_PRZERWANIE_INT)


	//--- PRESKALER -----------------------------------------

	//automatyczne dobranie parametr�w do ustawienia w rejestrze TCCR0
	#if   DD_RC5_TIMER_PRESKALER == 1
		#define DD_RC5_TIMER_PRESKALER_BITY	((1<<CS00))
	#elif DD_RC5_TIMER_PRESKALER == 8
		#define DD_RC5_TIMER_PRESKALER_BITY	((1<<CS01))
	#elif DD_RC5_TIMER_PRESKALER == 64
		#define DD_RC5_TIMER_PRESKALER_BITY	((1<<CS01)|(1<<CS00))
	#elif DD_RC5_TIMER_PRESKALER == 256
		#define DD_RC5_TIMER_PRESKALER_BITY	((1<<CS02))
	#elif DD_RC5_TIMER_PRESKALER == 1024
		#define DD_RC5_TIMER_PRESKALER_BITY	((1<<CS02)|(1<<CS00))
	#endif


	
	/* CZASY BIT�W I PӣBIT�W */
	
	//Czasy bit�w i p�bit�w obliczone zgodnie z wybran� tolerancj�
	//1778us - czas trwania bitu w kodowaniu RC5
#if (1==1)	
	#define DD_RC5_OKRES_BITU_MIN \
    	        ((F_CPU/1000UL) * (1778UL-DD_RC5_TOLERANCJA_US) / \
        	    (DD_RC5_TIMER_PRESKALER * 1000UL))
	
	#define DD_RC5_OKRES_BITU_MAX \
    	        ((F_CPU/1000UL) * (1778UL+DD_RC5_TOLERANCJA_US) / \
        	    (DD_RC5_TIMER_PRESKALER * 1000UL))

	#define DD_RC5_OKRES_POLOWY_BITU_MIN \
    	        (((F_CPU/1000UL) * ((1778UL-DD_RC5_TOLERANCJA_US)/2)/ \
        	    (DD_RC5_TIMER_PRESKALER * 1000UL)))

	#define DD_RC5_OKRES_POLOWY_BITU_MAX \
    	        (((F_CPU/1000UL) * ((1778UL+DD_RC5_TOLERANCJA_US)/2)  / \
        	    (DD_RC5_TIMER_PRESKALER * 1000UL)))
#endif
#if 0
#define DD_RC5_OKRES_BITU_MIN	57
#define DD_RC5_OKRES_BITU_MAX	76
#define DD_RC5_OKRES_POLOWY_BITU_MIN	29
#define DD_RC5_OKRES_POLOWY_BITU_MAX	38
#endif


	//Sprawdzamy, czy obliczony stan timera dla maksymalnego trwania bitu
	//nie przekroczy mo�liwo�ci timera (8-bit) poniewa� nie obs�ugujemy 
	//jego przepe�nienia
	#if (DD_RC5_OKRES_BITU_MAX > 65535)
		# warning "DD_RC5: Preskaler timer0 jest zbyt maly w stosunku do czestotliwosci F_CPU lub ustawiles zbyt duza tolerancje"
		# warning "DD_RC5: Dekodowanie RC_5 nie bedzie dzialac prawidlowo."
		# warning "DD_RC5: Zwieksz preskaler timera i/lub parametr tolerancji."
		# warning "DD_RC5: Szczegoly opiasne sa w tutaj: http://mikrokontrolery.blogspot.com/2011/03/RC5-IR-podczerwien-odbior-danych-przyklad-AVR-ATmega8.html"
	#endif

	//sprawdzamy, czy obliczony stan timera dla po�owy bitu b�dzie r�wny 
	//co najmniej 2. Lepiej nie dzia�a� na granicy mo�liwo��i timera.
	#if (DD_RC5_OKRES_POLOWY_BITU_MIN < 2)
		# warning "DD_RC5: Preskaler timer0 jest zbyt duzy w stosunku do czestotliwosci F_CPU"
		# warning "DD_RC5: Dekodowanie RC_5 nie bedzie dzialac prawidlowo."
		# warning "DD_RC5: Zmniejsz preskaler timera i/lub zwieksz czestotliwosc zegara mikrokontrolera."
		# warning "DD_RC5: Szczegoly opiasne sa w tutaj: http://mikrokontrolery.blogspot.com/2011/03/RC5-IR-podczerwien-odbior-danych-przyklad-AVR-ATmega8.html"
	#endif


	//Sprawdzamy, czy obliczone warto�ci timera dla minimalnego czasu trwania
	//ca�ego bitu nie zaz�biaj� si� z czasem maksymalnego trwania p�bitu
	#if (DD_RC5_OKRES_BITU_MIN <= DD_RC5_OKRES_POLOWY_BITU_MAX)
		# warning "DD_RC5: Czas minimalnego trwania calego bitu, zachodzi na maksymalny czas trwania polowy bitu."
		# warning "DD_RC5: Zmniejsz wartosc parametru DD_RC5_TOLERANCJA_US_PROC."
		# warning "DD_RC5: Szczegoly opiasne sa w tutaj: http://mikrokontrolery.blogspot.com/2011/03/RC5-IR-podczerwien-odbior-danych-przyklad-AVR-ATmega8.html"
	#endif


	//Sprawdzamy, czy wybrany poziom tolerancji jest wystarczaj�cy 
	//do poprawnego dekodowania sygna�u
	#if (DD_RC5_TOLERANCJA_US < 150)
		# warning "DD_RC5: Wybrany poziom tolerancji jest maly przez co jest spore prawdopodobienstwo, ze transmisja nie bedzie dzialac."
		# warning "DD_RC5: Zwieksz wartosc parametru DD_RC5_TOLERANCJA_US."
		# warning "DD_RC5: Mozesz zingnorowac ten warning bedac swiadomym skutkow."
		# warning "DD_RC5: Szczegoly opiasne sa w tutaj: http://mikrokontrolery.blogspot.com/2011/03/RC5-IR-podczerwien-odbior-danych-przyklad-AVR-ATmega8.html"
	#endif



	/*--- DEKLARACJE FUNKCJI ---------------------------------------------*/
	void dd_rc5_ini(void);



	/*--- DEKLARACJE ZMIENNYCH GLOBALNYCH --------------------------------*/

	//Zmienna: dd_rc5_dane_odebrane
	extern volatile unsigned char dd_rc5_dane_odebrane; //dane odebrane z pilota

	//Zmienna: dd_rc5_status
	//zawiera w sobie:
	// bity 0-4 - adres urz�dzenia
	// bit  5	- stan bitu toggle
	// bit  6	- nie wykorzystany
	// bit  7	- flaga poprawnego odebrania komendy z pilota i gotowo�ci
	//			  do odczytu ze zmiennej dd_rc5_komenda. 
	//			  UWAGA! 
	//			  Po dokonaniu odczytu odebranej danej i ewentualnie statusu,
	//			  nale�y wyzerowa� ten bit (bit 7), aby funkcja odbieraj�ca 
	//			  rozpocz�a nas�uchiwanie nowych danych z pilota.
	//			  Je�eli nie wyzerujesz bitu 7, nowe dane nie b�d� odbierane.
	//			  Zamiast zerowa� tylko bit 7, mo�esz wyzerowa� ca�� 
	//			  zmienn� dd_rc5_status
	extern volatile unsigned char dd_rc5_status;
	//maska bitu 7 rejestru statusu dd_rc5_status
	#define DD_RC5_STATUS_DANE_GOTOWE_DO_ODCZYTU 0x80 //bit 7
	#define DD_RC5_TOGGLE 0x20


#endif /* _DD_RC5_H_ */

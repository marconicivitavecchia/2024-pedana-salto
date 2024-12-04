
>[Torna all'indice](readme.md#fasi-progetto)

Lo spettro (trasformata di Fourier) del segnale campionato è una **ripetizione periodica** dello **spettro base** del segnale tempo continuo che si estende teoricamente all'**infinito** ma comunque praticamente ben oltre la massima freuenza utile del segnale campionato. 

<img src="img\aliasing-spectrum.png" alt="alt text" width="800">

Il problema è che si tratta di una ripetizione che ha due caratteristiche entrambe negative:
- vale per tutte le frequenze campionate dall'ADC, quindi anche per quelle ben **al di sopra** del **segnale base**
- per una certa banda campionata, si estende in **entrambe le direzioni** dello spettro, quindi questa viene **traslata** sia in alto che in basso, e quindi potenzialmente **pure in banda base**

Se un segnale contiene frequenze superiori a fs/2 (frequenza di Nyquist), queste frequenze vengono, per effetto del campionamento, "ripiegate", cioè traslate, nello **spettro base**, creando frequenze fantasma le cui componenti si sovrappongono al segnale utile **distorcendolo** irreversibilmente e quindi peggiorando il **rapporto segnale rumore** in ingresso.

<img src="img\aliasing.png" alt="alt text" width="900">

Sotto certe condizioni il filtro antialiasing è **non necessario** o molto rilassato nel progetto. E' il caso in cui la frequenza di campionamento fs è **molto maggiore** (es. 10-20 volte) della massima frequenza del segnale, allora le componenti alias cadrebbero in **bande molto alte**, dove il segnale è già **naturalmente attenuato** dal normale comportamente passa basso che tutti i dispositivi reali posseggono.

I convertitori sigma-delta relizzano un sovracampionamento intrinseco a frequenza interna molto alta (MHz) OSR tipicamente > 64 volte la frequenza di campionamento Fc. Ne segue che le componenti alias sono spostate a frequenze molto elevate e quindi soggette ad **attenuazione naturale**.

Questo è in larga parte il caso di tutti i convertitori sigma-delta che quindi hanno la proprietà di richiedere filtri antialiasing passa basso dalle **specifiche molto rilassate** (filtri BP del primo ordine).

<img src="img\delta-sigma-filter.png" alt="alt text" width="1000">

Tra il sensore e il convertitore delta-sigma è presente un **filtro passa-basso anti-aliasing**. Il design del convertitore delta-sigma e del filtro anti-aliasing SAR-ADC è significativamente **diverso**. Con un convertitore SAR, il filtro anti-aliasing solitamente ha un'implementazione attiva dal **quarto all'ottavo ordine**, che richiede da due a quattro amplificatori. Come mostra la Figura, l'implementazione dell'**anti-aliasing delta-sigma** richiede generalmente solo un filtro passivo del **primo ordine**.

Una volta stabilita la **frequenza di antialiasing target di FD**, è possibile definire rapidamente le formule di progettazione teorica. Il calcolo per questa valutazione teorica tiene conto del **rumore del resistore** e dei **bit del convertitore**. 

Per determinare la **resistenza teorica** del filtro, utilizzare la seguente equazione:

$$R_{FIT(MAX)} = \frac{10^{-(ER \times 0.602)}}{4 \times k \times T \times F_D}$$

dove:

R_FIT(MAX) è la resistenza massima
ER è l'error rate
k è la costante di Boltzmann
T è la temperatura assoluta
FD è il fattore di duty cycle

$$C_{FIT} = \frac{1}{2\pi \times R_{FIT} \times F_D}$$

Si noti che i circuiti e le discussioni presentate riguardano solo la riduzione del **rumore differenziale**, senza tenere conto dell'**impedenza di ingresso** del convertitore o del **rumore di modo comune**.

Si può dimostrare che l'impedenza di ingresso media di un ADC delta-sigma apparentemente sembra essere **resistiva** e dipende da circuiterie di filtraggio interne particolari per questo tipo di convertitori (filtri commutati).

- Per misurare l'**impedenza di ingresso in modo comune** della struttura in figura, collegare AIN P e AIN N insieme e misurare la corrente media che ogni pin consuma durante la conversione. 

- Per misurare l'**impedenza di ingresso differenziale**, applicare un segnale differenziale ad AIN P e AIN N e misurare la corrente media che scorre attraverso il pin a V A.

La resistenza in modo comune e differenziale può variare da centinaia di kilohm a centinaia di megaohm. Il valore di R FLT /2 deve essere almeno 10 volte inferiore alle impedenze di ingresso del convertitore.

I due condensatori di modo comune, C CM_P e C CM_N , attenuano il rumore di modo comune ad alta frequenza. Il **condensatore differenziale** dovrebbe essere almeno di un ordine di grandezza **maggiore** dei **condensatori di modo comune** perché le discrepanze nei condensatori di modo comune causano rumore differenziale.

**Sitografia:**
- https://www.edn.com/delta-sigma-antialiasing-filter-with-a-mode-rejection-circuit/
- https://www.edn.com/analog-filter-eases-delta-sigma-converter-design/
- https://www.planetanalog.com/adc-basics-part-4-using-delta-sigma-adcs-in-your-design/
- https://www.edn.com/delta-sigma-antialiasing-filter-with-a-mode-rejection-circuit/
- https://www.planetanalog.com/adc-basics-part-1-does-your-adc-work-in-the-real-world/


>[Torna all'indice](readme.md#fasi-progetto)

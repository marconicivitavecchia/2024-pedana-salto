
>[Torna all'indice](readme.md#fasi-progetto)


I convertitori delta-sigma sono disponibili con molte funzionalità aggiuntive che li rendono ideali per l'acquisizione dati. Molti di questi tipi di convertitori includono un amplificatore a guadagno programmabile (PGA) e un buffer di ingresso che possono ridurre ulteriormente i requisiti per il condizionamento del segnale esterno. Alcuni hanno anche caratteristiche speciali per i collegamenti dei sensori, come le fonti di corrente di burnout.

Le applicazioni del convertitore delta-sigma hanno meno componenti rispetto ai circuiti SAR-ADC. Durante il funzionamento, l'ADC delta-sigma sovracampiona continuamente un segnale di tensione in ingresso. L'ADC applica quindi un filtro digitale su questi campioni per ottenere un'uscita digitale multi-bit e a basso rumore. Il sottoprodotto di questo algoritmo è una gamma dinamica più elevata e velocità di uscita inferiori. Molti progettisti si concentrano sul numero di bit di output che questo tipo di convertitore può produrre. Tuttavia, la caratteristica nascosta spesso trascurata è il guadagno di processo. Questa caratteristica consente al progettista di eliminare i circuiti analogici esterni in queste catene di segnali a bassa frequenza.

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

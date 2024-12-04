
>[Torna all'indice](readme.md#fasi-progetto)


I convertitori delta-sigma sono disponibili con molte funzionalità aggiuntive che li rendono ideali per l'acquisizione dati. Molti di questi tipi di convertitori includono un amplificatore a guadagno programmabile (PGA) e un buffer di ingresso che possono ridurre ulteriormente i requisiti per il condizionamento del segnale esterno. Alcuni hanno anche caratteristiche speciali per i collegamenti dei sensori, come le fonti di corrente di burnout.

Le applicazioni del convertitore delta-sigma hanno meno componenti rispetto ai circuiti SAR-ADC. Durante il funzionamento, l'ADC delta-sigma sovracampiona continuamente un segnale di tensione in ingresso. L'ADC applica quindi un filtro digitale su questi campioni per ottenere un'uscita digitale multi-bit e a basso rumore. Il sottoprodotto di questo algoritmo è una gamma dinamica più elevata e velocità di uscita inferiori. Molti progettisti si concentrano sul numero di bit di output che questo tipo di convertitore può produrre. Tuttavia, la caratteristica nascosta spesso trascurata è il guadagno di processo. Questa caratteristica consente al progettista di eliminare i circuiti analogici esterni in queste catene di segnali a bassa frequenza.

<img src="img\delta-sigma-filter.png" alt="alt text" width="1000">

Tra il sensore e il convertitore delta-sigma è presente un filtro passa-basso anti-aliasing. Il design del convertitore delta-sigma e del filtro anti-aliasing SAR-ADC è significativamente diverso. Con un convertitore SAR, il filtro anti-aliasing solitamente ha un'implementazione attiva dal quarto all'ottavo ordine, che richiede da due a quattro amplificatori. Come mostra la Figura 1, l'implementazione dell'anti-aliasing delta-sigma richiede generalmente solo un filtro passivo del primo ordine [1] .





Sitografia:
- https://www.edn.com/analog-filter-eases-delta-sigma-converter-design/
- https://www.planetanalog.com/adc-basics-part-4-using-delta-sigma-adcs-in-your-design/
- https://www.edn.com/delta-sigma-antialiasing-filter-with-a-mode-rejection-circuit/
- https://www.planetanalog.com/adc-basics-part-1-does-your-adc-work-in-the-real-world/


>[Torna all'indice](readme.md#fasi-progetto)

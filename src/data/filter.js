class Filter {
    constructor() {
        this.gain = 1.0;
    }
 
    predict() {
        // Nessuna predizione per filtro base
        return this.gain;
    }
 
    update(measurement) {
        return this.gain * measurement;
    }
 
    setGain(gain) {
        this.gain = gain;
    }
 }

 class EMAFilter extends Filter {
    constructor(alpha = 0.1) {
        super();
        this.alpha = alpha;
        this.lastValue = null;
    }
 
    update(measurement) {
        if (this.lastValue === null) {
            this.lastValue = measurement;
            return measurement;
        }
        this.lastValue = this.alpha * measurement + (1 - this.alpha) * this.lastValue;
        return this.lastValue;
    }
 
    setAlpha(alpha) {
        this.alpha = alpha;
    }
 
    reset() {
        this.lastValue = null;
    }
 }

// kalman_base.js
class KalmanFilter {
    /*
    Equazioni del filtro di Kalman:
    
    Stato: x[k]
    Misura: z[k] = H * x[k] + v[k]    dove v[k] ~ N(0,R)
    Modello: x[k+1] = F * x[k] + w[k]  dove w[k] ~ N(0,Q)

    Predizione:
    x̂[k|k-1] = F * x̂[k-1|k-1]               // stima a priori
    P[k|k-1] = F * P[k-1|k-1] * F' + Q      // covarianza a priori

    Correzione:
    y[k] = z[k] - H * x̂[k|k-1]              // innovazione
    S[k] = H * P[k|k-1] * H' + R            // covarianza innovazione  
    K[k] = P[k|k-1] * H' * inv(S[k])        // guadagno di Kalman
    x̂[k|k] = x̂[k|k-1] + K[k] * y[k]         // stima a posteriori
    P[k|k] = (I - K[k] * H) * P[k|k-1]      // covarianza a posteriori
    */
    constructor(state_size, measurement_size) {
        this.state_size = state_size;
        this.measurement_size = measurement_size;
        this.x = Matrix.zeros(state_size, 1);      // x̂[k|k]
        this.P = Matrix.eye(state_size);           // P[k|k]
    }
    
    initModel() {
        // Le classi derivate definiscono:
        this.F = Matrix.eye(this.state_size);        // Matrice dinamica del sistema
        this.H = Matrix.zeros(this.measurement_size, this.state_size); // Matrice di misura
        this.Q = Matrix.eye(this.state_size);        // Covarianza rumore processo w[k]
        this.R = Matrix.eye(this.measurement_size);   // Covarianza rumore misura v[k]
    }

    predict() {
        // x̂[k|k-1] = F * x̂[k-1|k-1]
        this.x = this.F.multiply(this.x);
        
        // P[k|k-1] = F * P[k-1|k-1] * F' + Q
        this.P = this.F.multiply(this.P)
                     .multiply(this.F.transpose())
                     .add(this.Q);
        return this.x;
    }

    update(measurement) {
        // z[k] - misura corrente
        const z = Matrix.from2DArray([[measurement]]);
        
        // y[k] = z[k] - H * x̂[k|k-1]
        const y = z.subtract(this.H.multiply(this.x));
        
        // S[k] = H * P[k|k-1] * H' + R
        const S = this.H.multiply(this.P)
                       .multiply(this.H.transpose())
                       .add(this.R);
        
        // K[k] = P[k|k-1] * H' * inv(S[k])
        const K = this.P.multiply(this.H.transpose())
                       .multiply(S.inverse());
        
        // x̂[k|k] = x̂[k|k-1] + K[k] * y[k]
        this.x = this.x.add(K.multiply(y));
        
        // P[k|k] = (I - K[k] * H) * P[k|k-1]
        const I = Matrix.eye(this.state_size);
        this.P = I.subtract(K.multiply(this.H))
                 .multiply(this.P);
        return this.x;
    }
}

class AccelerationKalmanFilter extends KalmanFilter {
    constructor(dt = 0.001) {
        super(3, 1);
        this.dt = dt;
        this.process_noise = 0.1;
        this.initModel();
    }
 
    initModel() {
        this.F = Matrix.from2DArray([
            [1, this.dt, this.dt*this.dt/2],
            [0, 1, this.dt],
            [0, 0, 1]
        ]);
        
        this.H = Matrix.from2DArray([[0, 0, 1]]);
        
        const Q_base = Matrix.from2DArray([
            [this.dt**4/4, this.dt**3/2, this.dt**2/2],
            [this.dt**3/2, this.dt**2, this.dt],
            [this.dt**2/2, this.dt, 1]
        ]);
        
        this.Q = Q_base.multiplyScalar(this.process_noise);
        this.R = Matrix.from2DArray([[1]]); 
    }
 
    getPosition() {
        return this.x.data[0][0];
    }
 
    getVelocity() {
        return this.x.data[1][0];  
    }
 
    getAcceleration() {
        return this.x.data[2][0];
    }
 }

class JumpHeightKalmanFilter extends KalmanFilter {
    constructor(dt = 0.001, measurement_noise = 0.1) {
        super(2, 1); // stato [altezza, tempo_volo]
        this.dt = dt;
        this.g = 9.81;
        this.measurement_noise = 0.1; // Aggiungiamo il parametro mancante
        this.initModel();
    }

    initModel() {
        // Modello fisico: h = g*t²/8
        this.F = Matrix.from2DArray([
            [1, this.g * this.dt / 4],
            [0, 1]
        ]);
        
        // Misuriamo solo il tempo di volo
        this.H = Matrix.from2DArray([[0, 1]]);
        
        this.Q = Matrix.from2DArray([
            [0.01, 0],
            [0, 0.01]
        ]);
        
        this.R = Matrix.from2DArray([[this.measurement_noise]]);
    }

    getHeight() {
        return this.x.data[0][0];
    }

    getFlightTime() {
        return this.x.data[1][0];
    }
}
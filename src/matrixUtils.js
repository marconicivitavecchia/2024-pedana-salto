// matrix_utils.js
class Matrix {
    constructor(rows, cols) {
        this.rows = rows;
        this.cols = cols;
        this.data = Array(rows).fill().map(() => Array(cols).fill(0));
    }

    static eye(size) {
        const m = new Matrix(size, size);
        for (let i = 0; i < size; i++) {
            m.data[i][i] = 1;
        }
        return m;
    }

    static zeros(rows, cols) {
        return new Matrix(rows, cols);
    }

    static from2DArray(array) {
        const m = new Matrix(array.length, array[0].length);
        m.data = array.map(row => [...row]);
        return m;
    }

    add(other) {
        const result = new Matrix(this.rows, this.cols);
        for (let i = 0; i < this.rows; i++) {
            for (let j = 0; j < this.cols; j++) {
                result.data[i][j] = this.data[i][j] + other.data[i][j];
            }
        }
        return result;
    }

    subtract(other) {
        const result = new Matrix(this.rows, this.cols);
        for (let i = 0; i < this.rows; i++) {
            for (let j = 0; j < this.cols; j++) {
                result.data[i][j] = this.data[i][j] - other.data[i][j];
            }
        }
        return result;
    }

    multiply(other) {
        if (this.cols !== other.rows) {
            throw new Error('Matrix dimensions do not match for multiplication');
        }
        const result = new Matrix(this.rows, other.cols);
        for (let i = 0; i < this.rows; i++) {
            for (let j = 0; j < other.cols; j++) {
                let sum = 0;
                for (let k = 0; k < this.cols; k++) {
                    sum += this.data[i][k] * other.data[k][j];
                }
                result.data[i][j] = sum;
            }
        }
        return result;
    }

    transpose() {
        const result = new Matrix(this.cols, this.rows);
        for (let i = 0; i < this.rows; i++) {
            for (let j = 0; j < this.cols; j++) {
                result.data[j][i] = this.data[i][j];
            }
        }
        return result;
    }

    inverse() {
        if (this.rows !== this.cols) {
            throw new Error('Matrix must be square for inverse');
        }
        if (this.rows === 1) {
            const result = new Matrix(1, 1);
            result.data[0][0] = 1 / this.data[0][0];
            return result;
        }
        throw new Error('Only 1x1 matrix inverse implemented');
    }

    // In Matrix class
    multiplyScalar(scalar) {
        const result = new Matrix(this.rows, this.cols);
        for (let i = 0; i < this.rows; i++) {
            for (let j = 0; j < this.cols; j++) {
                result.data[i][j] = this.data[i][j] * scalar;
            }
        }
        return result;
    }
}
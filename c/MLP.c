#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "LogisticRegression.h"
#include "HiddenLayer.h"
#include "MLP.h"

double uniform(double min, double max) {
	return rand() / (RAND_MAX + 1.0) * (max - min) + min;
}
void test_mlp(void);


double sigmoid(double x) {
	return 1.0 / (1.0 + exp(-x));
}

// MLP
void MLP__construct(MLP* this, int N, int n_ins,\
				   int *hidden_layer_sizes, int n_outs, int n_layers) {
	int i, input_size;

	this->N = N;
	this->n_ins = n_ins;
	this->n_outs = n_outs;
	this->n_layers = n_layers;

	this->hidden_layers = (HiddenLayer *)malloc(sizeof(HiddenLayer) * n_layers);
	
	// construct multi-layer
	for(i=0; i<n_layers; i++) {
		if(i == 0) {
			input_size = n_ins;
		} else {
			input_size = hidden_layer_sizes[i-1];
		}

		// construct sigmoid_layer
		HiddenLayer__construct(&(this->hidden_layers[i]), N, \
							   input_size, hidden_layer_sizes[i], NULL, NULL);
	}

	// layer for output using LogisticRegression
	LogisticRegression__construct(&(this->log_layer), N, hidden_layer_sizes[n_layers-1], n_outs);

}

void MLP__destruct(MLP* this) {
	int i;
	for(i=0; i<this->n_layers; i++) {
		HiddenLayer__destruct(&(this->hidden_layers[i]));
	}
	free(this->hidden_layers);
	LogisticRegression__destruct(&this->log_layer);
}

void MLP_train(MLP* this, int *input, int *label, double lr, double *cost) {
	int i, j, k;
	double **py = (double**)malloc(sizeof(double*) * (this->n_layers+1));
	for(i=0; i<=this->n_layers; i++) {
		if (i != this->n_layers) {
			py[i] = (double*)malloc(sizeof(double) * this->hidden_layers[i].n_out);
		} else {
			py[i] = (double*)malloc(sizeof(double) * this->n_outs);
		}
	}
	double **dy = (double**)malloc(sizeof(double*) * (this->n_layers+1));
	for(i=0; i<=this->n_layers; i++) {
		if (i != this->n_layers) {
			dy[i] = (double*)malloc(sizeof(double) * this->hidden_layers[i].n_out);
		} else {
			dy[i] = (double*)malloc(sizeof(double) * this->n_outs);
		}
	}

	MLP_predict(this, input, NULL, py, NULL, cost);

	for(i=0; i<this->n_outs; i++) {
		dy[this->n_layers][i] = (double)label[i] - py[this->n_layers][i];
	}

	for(i=0; i<this->log_layer.n_in; i++) {
		for(j=0; j<this->log_layer.n_out; j++) {
			dy[this->n_layers-1][i] += dy[this->n_layers][j] * this->log_layer.W[j][i];
		}
	}

	for(i=0; i<this->n_outs; i++) {
		for(j=0; j<this->log_layer.n_in; j++) {
			this->log_layer.W[i][j] += lr * dy[this->n_layers][i] * py[this->n_layers-1][j] / this->N;
		}
		this->log_layer.b[i] += lr * dy[this->n_layers][i] / this->N;
	}

	for(i=this->n_layers-1; i>=0; i--) {
		if(i != 0) {
			for(j=0; j<this->hidden_layers[i].n_in; j++) {
				for(k=0; k<this->hidden_layers[i].n_out; k++) {
					dy[i-1][j] += dy[i][k] * this->hidden_layers[i].W[k][j];
				}
				dy[i-1][j] *= py[i-1][j] * (1 - py[i-1][j]);
			}
		}
		for(j=0; j<this->hidden_layers[i].n_out; j++) {
			for(k=0; k<this->hidden_layers[i].n_in; k++) {
				this->hidden_layers[i].W[j][k] += lr * dy[i+1][j] * py[i][k] / this->N;
			}
			this->hidden_layers[i].b[j] += lr * dy[i+1][j] / this->N;
		}
	}
	
	for(i=0; i<=this->n_layers; i++) {
		free(py[i]);
		free(dy[i]);
	}
	free(py);
	free(dy);
}

void MLP_predict(MLP *this, int *input, double *pred, double **py, int *label, double *cost) {
	int i, j, train;
	train = 1;
	if (py == NULL) {
		train = 0;
		py = (double**)malloc(sizeof(double*) * (this->n_layers+1));
		for(i=0; i<=this->n_layers; i++) {
			if(i != this->n_layers) {
				py[i] = (double*)malloc(sizeof(double) * this->hidden_layers[i].n_out);
			} else {
				py[i] = (double*)malloc(sizeof(double) * this->log_layer.n_out);
			}
		}
	} 

	if(this->n_layers != 0) {
		for(i=0; i<this->hidden_layers[0].n_out; i++) {
			py[0][i] = HiddenLayer_output(&this->hidden_layers[0], input, this->hidden_layers[0].W[i], this->hidden_layers[0].b[i]);
		}
	}
	for(i=1; i<this->n_layers; i++) {
		for(j=0; j<this->hidden_layers[i].n_out; j++) {
			py[i][j] = HiddenLayer_output(&this->hidden_layers[i], py[i-1], this->hidden_layers[i].W[j], this->hidden_layers[i].b[j]);
		}
	}
	LogisticRegression_predict(&this->log_layer, py[this->n_layers-1], py[this->n_layers]);

	if (train==0) {
		for(i=0; i<this->log_layer.n_out; i++) {
			pred[i] = py[this->n_layers][i];
			*cost += -label[i] * log(pred[i]);
		}
		
		for(i=0; i<=this->n_layers; i++) {
			free(py[i]);
		}
		free(py);
	}
}

void LogisticRegression__construct(LogisticRegression *this, int N, int n_in, int n_out) {
	int i, j;
	this->N = N;
	this->n_in = n_in;
	this->n_out = n_out;

	this->W = (double **)malloc(sizeof(double*) * n_out);
	this->W[0] = (double *)malloc(sizeof(double*) * n_in * n_out);
	for(i=0; i<n_out; i++) this->W[i] = this->W[0] + i*n_in;
	this->b = (double *)malloc(sizeof(double) * n_out);

	for(i=0; i<n_out; i++) {
		for(j=0; j<n_in; j++) {
			this->W[i][j] = 0;
		}
		this->b[i] = 0;
	}
}

void LogisticRegression__destruct(LogisticRegression *this) {
	free(this->W[0]);
	free(this->W);
	free(this->b);
}

void LogisticRegression_softmax(LogisticRegression *this, double *x) {
	int i;
	double max = 0.0;
	double sum = 0.0;

	for(i=0; i<this->n_out; i++) if(max < x[i]) max = x[i];
	for(i=0; i<this->n_out; i++) {
		if(x[i] - max < 45) { 
			x[i] = exp(x[i] - max);
		} else {
			x[i] = exp(45.0);
		}
		sum += x[i];
	}

	for(i=0; i<this->n_out; i++) {
		x[i] /= sum;
		printf("%lf ", x[i]);
	}
	printf("\n");
}

void LogisticRegression_predict(LogisticRegression *this, int *x, double *y) {
	int i, j;

	for(i=0; i<this->n_out; i++) {
		y[i] = 0;
		for(j=0; j<this->n_in; j++) {
			y[i] += this->W[i][j] * x[j];
		}
		y[i] += this->b[i];
	}

	LogisticRegression_softmax(this, y);
}

// HiddenLayer
void HiddenLayer__construct(HiddenLayer* this, int N, int n_in, int n_out, \
							double **w, double *b) {
	int i, j;
	double a = 1.0 / n_in;

	this->N = N;
	this->n_in = n_in;
	this->n_out = n_out;

	this->W = (double **)malloc(sizeof(double*) * n_out);
	this->W[0] = (double *)malloc(sizeof(double) * n_in * n_out);
	for(i=0; i<n_out; i++) this->W[i] = this->W[0] + i * n_in;

	for(i=0; i<n_out; i++) {
		for(j=0; j<n_in; j++) {
			this->W[i][j] = uniform(-a, a);
		}
	}

	this->b = (double *)malloc(sizeof(double) * n_out);
}

void HiddenLayer__destruct(HiddenLayer* this) {
	free(this->W[0]);
	free(this->W);
	free(this->b);
}

double HiddenLayer_output(HiddenLayer* this, double *input, double *w, double b) {
	int j;
	double linear_output = 0.0;
	for(j=0; j<this->n_in; j++) {
		linear_output += w[j] * input[j];
	}
	linear_output += b;
	return sigmoid(linear_output);
}

void test_mlp(void) {
	srand(0);

	int i, j, epoch;

	double *cost;
	double lr = 0.1;
	int n_epochs = 50;
	int train_N = 6;
	int test_N = 4;
	int n_ins = 6;
	int n_outs = 2;
	int hidden_layer_sizes[] = {3, 3};
	int n_layers = sizeof(hidden_layer_sizes) / sizeof(hidden_layer_sizes[0]);

	//training data
	int train_X[6][6] = {
		{1, 1, 1, 0, 0, 0},
		{1, 0, 1, 0, 0, 0},
		{1, 1, 1, 0, 0, 0},
		{0, 0, 1, 1, 1, 0},
		{0, 0, 1, 1, 0, 0},
		{0, 0, 1, 1, 1, 0}
	};

	int train_Y[6][2] = {
		{1, 0},
		{1, 0},
		{1, 0},
		{0, 1},
		{0, 1},
		{0, 1}
	};
	// construct MLP
	MLP mlp;
	MLP__construct(&mlp, train_N, n_ins, hidden_layer_sizes, n_outs, n_layers);
	
	printf("Train...\n");

	// train
	for(epoch=0; epoch<n_epochs; epoch++) {
		*cost = 0;
		for(i=0; i<train_N; i++) {
			MLP_train(&mlp, train_X[i], train_Y[i], lr, cost);
		}
		printf("Epoch %d: cost %lf\n", epoch, *cost);
		for(i=0; i<n_outs; i++) {
			printf("%lf ", mlp.log_layer.W[i][0]);
		}
		printf("\n");
	}

	// test data
	int test_X[4][6] = {
		{1, 0, 1, 0, 0, 0},
		{1, 1, 1, 0, 0, 0},
		{0, 0, 1, 1, 1, 0},
		{0, 0, 1, 1, 0, 0}
	};

	double test_Y[4][2];

	// test
	for(i=0; i<test_N; i++) {
		MLP_predict(&mlp, test_X[i], test_Y[i], NULL, NULL, NULL);
		for (j=0; j<n_outs; j++) {
			printf("%f ", test_Y[i][j]);
		}
		printf("\n");
	}

	// destruct MLP
	MLP__destruct(&mlp);
}


int main(void) {
	test_mlp();
	return 0;
}



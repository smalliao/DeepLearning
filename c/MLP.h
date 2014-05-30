#ifndef MLP_H
#define MLP_H

typedef struct {
	int N;
	int n_ins;
	int *hidden_layer_sizes;
	int n_outs;
	int n_layers;
	HiddenLayer *hidden_layers;
	LogisticRegression log_layer;
} MLP;

void MLP__construct(MLP*, int, int, int*, int, int);
void MLP__destruct(MLP*);
void MLP_train(MLP*, int*, int*, double);
void MLP_predict(MLP*, int*, double*, double **);
#endif



from scipy import ndimage

import numpy as np

class Reinforcement:

    def __init__(self, shape=50, speed=100, ini_c=3, var_c=2, scale=1, seed=0):
        self.width=1
        self.scale=scale
        self.shape=shape
        self.delta_t = 0.01 * speed / 100
        np.random.seed(seed)

        if type(self.shape) == int: self.shape = (self.shape, self.shape)

        self.c_reg = np.full(self.shape, ini_c, dtype=float)
        if var_c != 0: 
            self.c_reg += np.random.random_sample(self.shape) * var_c

        self.lap_c = np.empty_like(self.c_reg)

    def reinforcement_model(self, g):
        kernel_c = np.array([[1, 4, 1], [4, -20, 4], [1, 4, 1]]) / 6
        threshold = 1
        mc = self.c_reg
        wrap = True

        if wrap: 
            ndimage.convolve(mc, kernel_c, output=self.lap_c, mode='wrap')
        else:    
            ndimage.convolve(mc, kernel_c, output=self.lap_c, mode='reflect')

        self.c_reg = mc + ((threshold - self.width - mc) * (threshold - mc) * (threshold + self.width - mc) * g + self.scale * self.lap_c) * self.delta_t

    def load_c(self, c):
        self.c_reg = c

    def run_simulation(self, start, stop, increasing_size=False):
        gamma = 3 * np.sqrt(3) / (2 * self.width * self.width)
        
        for iteration in range(start + 1, stop + 1):
            if increasing_size: 
                if iteration % 50 == 1:
                    self.grow_one_row_c()
                    self.grow_one_col_c()

            if (self.c_reg).shape != self.shape:
                self.shape = self.c_reg.shape
                self.lap_c = np.empty_like(self.c_reg)
            
            self.reinforcement_model(gamma)

    def grow_one_row_c(self):
        rows, cols = self.c_reg.shape
        new_c = np.zeros((rows + 1, cols))
        new_c[:rows,:] = self.c_reg[:,:]
        for col in range(0, cols):
            row = np.random.randint(0, rows)
            # reduce division probability for middle rows
            if 0.45 * rows <= row <= 0.55 * rows:
                if np.random.random() < 0.5:
                    row = np.random.randint(0, rows)
            new_c[(row+1):(rows+1), col] = self.c_reg[row:rows, col]
        self.c_reg = new_c

    def grow_one_col_c(self):
        rows, cols = self.c_reg.shape
        new_c = np.zeros((rows, cols + 1))
        new_c[:,:cols] = self.c_reg[:,:]        
        for row in range(0, rows):
            col = np.random.randint(0, cols)
            new_c[row, (col+1):(cols+1)] = self.c_reg[row, col:cols]
        self.c_reg = new_c

    def getC(self):
        return self.c_reg
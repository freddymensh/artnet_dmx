# -*- coding: utf-8 -*-
"""
Created on Tue Jul  4 13:15:28 2023

@author: fredd
"""

import numpy as np
import numba as nb


def color_float_to_uint8(color):
    """
    

    Parameters
    ----------
    color : array of float
        colors in range (0, 1)

    Returns
    -------
    array of int8 and shape of param color
        colors in range (0, 255)

    """
    return np.uint8(color * 255)
    

def color_uint8_to_float32(color):
    """
    

    Parameters
    ----------
    color : array of uint8
        colors in range (0, 255)

    Returns
    -------
    array of int8 and shape of param color
        colors in range (0, 255)

    """
    return np.float32(color / 255)
    

@nb.njit([nb.float32[:](nb.float32[:], nb.float32[:])])
def alpha_blending(color_A, color_B):
    """
    Two colors are blended over each other with the porter-duff-algorithm.
    Color A lays on top of color B. So if A is opaque color B has no influence.
    
    https://en.wikipedia.org/wiki/Alpha_compositing#Description
    
    Parameters
    ----------
    color_A : array-like of shape (4,)
        RGBA-color.
    color_B : array-like of shape (4,)
        RGBA-color.

    Returns
    -------
    array-like of shape (4,)
        RGBA-color.

    """
    alpha = 3
    
    color_C = np.array_like(color_A)
    
    color_C[alpha] = color_A[alpha] + (1-color_A[alpha])*color_B[alpha]
    
    for C in nb.prange(0, color_A.shape[0]-1):
        color_C[C] = (1/color_C[alpha]) * ( color_A[alpha]*color_A[C] +
                                           (1-color_A[alpha])*color_B[alpha] * color_B[C]
                                          )
        
    return color_C
    
# -*- coding: utf-8 -*-
"""
Created on Tue Jul  4 13:44:49 2023

@author: fredd

sources:
    https://yellowsplash.wordpress.com/2009/10/23/fast-antialiased-circles-and-ellipses-from-xiaolin-wus-concepts/
    https://create.stephan-brumme.com/antialiased-circle/#antialiased-circle-wu
    https://stackoverflow.com/questions/54594822/xiaolin-wu-circle-algorithm-renders-circle-with-holes-inside

"""

import numpy as np
from numpy import round, floor, ceil, sqrt
from matplotlib import pyplot as plt
import numba as nb

NB_FASTMATH = True
NB_CACHE    = True
NB_PARALLEL = True

__all__ = ["draw_ellipse", "fill_ellipse", "hollow_ellipse"]


@nb.njit(fastmath=NB_FASTMATH, cache=NB_CACHE, parallel=False)
def setpixel4(img, centerX, centerY, deltaX, deltaY, color, skip_colors=(0,)):
    """
    set pixels in img. It uses symetries

    Parameters
    ----------
    img : np.ndarray of shape (n, m) 
        the image canvas
    centerX : int
        center of the ellipse - x coordinate
    centerY : int
        center of the ellipse - y coordinate
    deltaX : int
        distance from the center of the ellipse - x coordinate
    deltaY : int
        distance from the center of the ellipse - x coordinate
    color : img.dtype
        value (scalar) to be set on the canvas
    skip_colors : tuple of img.dtype, optional
        Colors being skipped. Set the background color here. Skipping is 
        necessary to avoid invalid indices internally.
        The default is (0,).

    Returns
    -------
    None.

    """
    if color in skip_colors:
        return
    else:
        img[centerX + deltaX, centerY + deltaY] = color
        img[centerX - deltaX, centerY + deltaY] = color
        img[centerX + deltaX, centerY - deltaY] = color
        img[centerX - deltaX, centerY - deltaY] = color
    

@nb.njit(fastmath=NB_FASTMATH, cache=NB_CACHE, parallel=False)
def _ellipse__alpha(value, maxTransparency):
    error = value - floor(value)
    transparency = error * maxTransparency
    alpha  = transparency
    alpha2 = maxTransparency - transparency
    return alpha, alpha2
    

@nb.njit(fastmath=NB_FASTMATH, cache=NB_CACHE, parallel=False)
def _ellipse__quarter(radiusA, radiusB):
     return int(round(radiusA**2 / sqrt(radiusA**2 + radiusB**2))) + 1


@nb.njit(fastmath=NB_FASTMATH, cache=NB_CACHE, parallel=False)
def _ellipse__coordinate(radiusA, radiusB, a):
    return radiusB * sqrt(1-(a**2/radiusA**2))


@nb.njit(fastmath=NB_FASTMATH, cache=NB_CACHE, parallel=False)
def _ellipse__center(radiusX, radiusY):
    return int(ceil(radiusX)), int(ceil(radiusY))


@nb.njit(fastmath=NB_FASTMATH, cache=NB_CACHE, parallel=False)
def _ellipse__shape(radiusX, radiusY):
    return (int(2*ceil(radiusX)+1), int(2*ceil(radiusY))+1)


@nb.njit(fastmath=NB_FASTMATH, cache=NB_CACHE, parallel=False)
def _ellipse__handle_img(img, radiusX, radiusY, maxTransparency):
    if img is None:
        shape = _ellipse__shape(radiusX, radiusY)
        img = np.zeros(shape, dtype=type(maxTransparency))
        centerX, centerY = _ellipse__center(radiusX, radiusY)
    else:
        centerX, centerY = int(floor(img.shape[0]/2)), int(floor(img.shape[1]/2))
     
    return img, centerX, centerY


@nb.njit(fastmath=NB_FASTMATH, cache=NB_CACHE, parallel=NB_PARALLEL)
def draw_ellipse(radiusX, radiusY, maxTransparency=np.float32(1.0), img=None):
    """
    Drawing the outline of an ellipse. It returns a single color-channel. You 
    can use the return as alpha-channel in your application.

    Parameters
    ----------
    radiusX : scalar float or int
        radius in x-direction.
    radiusY : scalar float or int
        radius in y-direction.
    maxTransparency : scalar float or int, optional
        The maximum value drawn on the canvas. It is opaque color. 
        The default is np.float32(1.0).
    img : np.ndarray of shape (n, m), optional
        The canvas. The minimum dimensions are provided by the function _ellipse__shape.
        The default is None.

    Returns
    -------
    img : np.ndarray of shape (n, m)
        The canvas with drawn shape

    """
    img, centerX, centerY = _ellipse__handle_img(img, radiusX, radiusY, maxTransparency)

    # upper and lower halves 
    quarter = _ellipse__quarter(radiusX, radiusY)
    for x in nb.prange(quarter):   
        
        y = _ellipse__coordinate(radiusX, radiusY, x)
        alpha, alpha2 = _ellipse__alpha(value=y, maxTransparency=maxTransparency)

        # set pixels
        setpixel4(img, centerX, centerY, x, int(floor(y)+1), alpha)
        setpixel4(img, centerX, centerY, x, int(floor(y)),   alpha2)
    
    # right and left halves 
    quarter = _ellipse__quarter(radiusY, radiusX)
    for y in nb.prange(quarter):
        
        x = _ellipse__coordinate(radiusY, radiusX, y)
        alpha, alpha2 = _ellipse__alpha(value=x, maxTransparency=maxTransparency)
        
        # set pixels
        setpixel4(img, centerX, centerY, int(floor(x)+1), y, alpha)
        setpixel4(img, centerX, centerY, int(floor(x)),   y, alpha2)
 
    return img


@nb.njit(fastmath=NB_FASTMATH, cache=NB_CACHE, parallel=NB_PARALLEL)
def fill_ellipse(radiusX, radiusY, maxTransparency=np.float32(1.0), img=None):
    """
    Drawing a filled out ellipse. It returns a single color-channel. You 
    can use the return as alpha-channel in your application.

    Parameters
    ----------
    radiusX : scalar float or int
        radius in x-direction.
    radiusY : scalar float or int
        radius in y-direction.
    maxTransparency : scalar float or int, optional
        The maximum value drawn on the canvas. It is opaque color. 
        The default is np.float32(1.0).
    img : np.ndarray of shape (n, m), optional
        The canvas. The minimum dimensions are provided by the function _ellipse__shape.
        The default is None.

    Returns
    -------
    img : np.ndarray of shape (n, m)
        The canvas with drawn shape
    """
    img, centerX, centerY = _ellipse__handle_img(img, radiusX, radiusY, maxTransparency)
    
    # draw outline
    img = draw_ellipse(radiusX, radiusY, maxTransparency, img)
    
    # fill inside
    for x in nb.prange(img.shape[0]):
        for y in nb.prange(img.shape[1]):
            if ((x-centerX)**2/radiusX**2 + (y-centerY)**2/radiusY**2) < 1:
                img[x, y] = maxTransparency
    
    return img


@nb.njit(fastmath=NB_FASTMATH, cache=NB_CACHE, parallel=NB_PARALLEL)
def hollow_ellipse(radiusX, radiusY, border, maxTransparency=np.float32(1.0), img=None):
    """
    Similar to draw_ellipse. But you set the border width here.

    Parameters
    ----------
    radiusX : scalar float or int
        radius in x-direction.
    radiusY : scalar float or int
        radius in y-direction.
    border : scalar float or int
        The width of the border. 
    maxTransparency : scalar float or int, optional
        The maximum value drawn on the canvas. It is opaque color. 
        The default is np.float32(1.0).
    img : np.ndarray of shape (n, m), optional
        The canvas. The minimum dimensions are provided by the function _ellipse__shape.
        The default is None.

    Returns
    -------
    img : np.ndarray of shape (n, m)
        The canvas with drawn shape
    """
    img, centerX, centerY = _ellipse__handle_img(img, radiusX, radiusY, maxTransparency)
    
    border -= 1
    if border == 0:
        return draw_ellipse(radiusX, radiusY, maxTransparency, img)
    assert border > 0
    if border >= min(radiusX, radiusY):
        return fill_ellipse(radiusX, radiusY, maxTransparency, img)
    
    # draw outlines
    img  = draw_ellipse(radiusX,        radiusY,        maxTransparency, img)
    img2 = draw_ellipse(radiusX-border, radiusY-border, maxTransparency, img)

    # fill inside
    centerX, centerY = _ellipse__center(radiusX, radiusY)
    for x in nb.prange(img.shape[0]):
        for y in nb.prange(img.shape[1]):
            a = ((x-centerX)**2/radiusX**2 + (y-centerY)**2/radiusY**2) < 1
            b = ( ((x-centerX)**2) / ((radiusX-border)**2) ) + ( ((y-centerY)**2) / ((radiusY-border)**2) ) > 1
            if a and b:
                img[x, y] = maxTransparency
    
    return img2




    


if __name__ == '__main__':
    def show(img):
        fig, ax = plt.subplots(1, 1)
        ax.matshow(img, cmap='Greys')
        ax.grid()
        fig.show()
    
    
    maxTransparency = np.float32(1.0)
    r1, r2, border = 50, 30, 8.3
    shape = _ellipse__shape(r1, r2)
    
    
    img = np.zeros(shape, dtype=type(maxTransparency))
    img[0, 0] = maxTransparency
    img = hollow_ellipse(r1, r2, border, maxTransparency, img)
    show(img)
    
    img = np.zeros(shape[::-1], dtype=type(maxTransparency))
    img[0, 0] = maxTransparency
    img = hollow_ellipse(r2, r1, border, maxTransparency, img)
    show(img)
    
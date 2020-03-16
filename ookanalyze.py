import tkinter as tk
from tkinter import filedialog as fd 

from matplotlib.backends.backend_tkagg import (
    FigureCanvasTkAgg, NavigationToolbar2Tk)
from matplotlib.figure import Figure

import numpy as np


global data, canvas, fig

data = []

def redrawCallback():
    global data
    gap = int(eGap.get())
    za = int(eZa.get())
    zb = int(eZb.get())
    oa = int(eOa.get())
    ob = int(eOb.get())
    
    dfs = []
    dataParts = []
    df = None
    da = None
    for di in data:
        if df is None:
            df = np.array([], dtype='int8')
            da = []
        if di[1] > gap:
            dfs.append(np.copy(df))
            df = None
            dataParts.append(da.copy())
            da = None
            continue
        df = np.append(df, np.full(di[1], di[0]))
        da.append(di.copy())
    if df is not None:
        dfs.append(np.copy(df))
        dataParts.append(da.copy())
        
    #print(dfs)

    fig.clear()
    fn = 1
    ax = None
    for df, da in zip(dfs, dataParts):
        t = np.arange(0, df.size, 1)
        if ax is None:
            ax = fig.add_subplot(len(dfs),1,fn)
            p = ax
        else:
            p = fig.add_subplot(len(dfs),1,fn, sharex = ax)
        p.plot(t,df)
        
        x = 0
        for i,j in zip(da[0::2], da[1::2]):
            if abs(i[1]-za)+abs(j[1]-zb) < abs(i[1]-oa)+abs(j[1]-ob):
                p.axvspan(x, x+za, color='#b38f00', alpha=0.5)
                p.axvspan(x+za, x+za+zb, color='#ffe066', alpha=0.5)
            else:
                p.axvspan(x, x+oa, color='#248f24', alpha=0.5)
                p.axvspan(x+oa, x+oa+ob, color='#85e085', alpha=0.5)
            x = x + i[1] + j[1]
        fn = fn + 1
    canvas.draw()
    
def fileOpenCallback():
    global data
    filename= fd.askopenfilename()
    content = ''
    # Read the signal
    with open(filename) as f:
       # perform file operations
        content = f.read()
        
    i = content.find(':', 1, 10)+2
    if i < 2: i = 0
    
    da = content[i:].split(']')
    data = []
    for di in da:
        if di[0] == '0':
            data.append([0,int(di[2:])])
        elif di[0] == '1':
            data.append([1,int(di[2:])])
    print(data)

root = tk.Tk()
root.wm_title("OOK analyzator")

sideFrame = tk.Frame(root)
sideFrame.pack(side=tk.RIGHT)
bottomFrame = tk.Frame(root)
bottomFrame.pack(side=tk.BOTTOM)

fig = Figure(figsize=(5, 4), dpi=100)
canvas = FigureCanvasTkAgg(fig, master=root)  # A tk.DrawingArea.
canvas.draw()
toolbar = NavigationToolbar2Tk(canvas, root)
toolbar.update()

quitButton = tk.Button(master=bottomFrame, text="Quit", command=root.quit)
openButton = tk.Button(master=bottomFrame, text='Open', command=fileOpenCallback)
quitButton.pack(side=tk.LEFT)
openButton.pack(side=tk.LEFT)

tk.Label(master=sideFrame, text="Zero").grid(row=0)
eZa = tk.Entry(master=sideFrame)
eZa.insert(0, '100')
eZa.grid(row=1)
eZb = tk.Entry(master=sideFrame)
eZb.insert(0, '700')
eZb.grid(row=2)
tk.Label(master=sideFrame, text="One").grid(row=3)
eOa = tk.Entry(master=sideFrame)
eOa.insert(0, '700')
eOa.grid(row=4)
eOb = tk.Entry(master=sideFrame)
eOb.insert(0, '100')
eOb.grid(row=5)
tk.Label(master=sideFrame, text="Gap").grid(row=6)
eGap = tk.Entry(master=sideFrame)
eGap.insert(0, '3000')
eGap.grid(row=7)
tk.Button(master=sideFrame, text="Redraw", command=redrawCallback).grid(row=9)

canvas.get_tk_widget().pack(side=tk.TOP, fill=tk.BOTH, expand=1)

tk.mainloop()

    
    
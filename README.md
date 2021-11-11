It's a small and simple OS based on:

* http://www.brokenthorn.com/Resources/OSDevIndex.html
* http://www.jamesmolloy.co.uk/tutorial_html
* https://subscribe.ru/catalog/comp.soft.myosdev

I created it using Windows 10/FASM/MinGW/Bochs/Visual Studio Code.  
Development notes (currently on Russian only) are in **[/docs/notes.pdf](/docs/notes.pdf)**.  

OS uses **protected mode** and **paging**.
It has:
- a simple bootloader
- switching to user mode
- simplest multitasking 
- one system call (woW!!!)
- some other basic things.


![Screeshot of OS](https://raw.githubusercontent.com/devsienko/c4os/master/docs/screen.png)

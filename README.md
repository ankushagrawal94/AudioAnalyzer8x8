AudioAnalyzer8x8
================

This code reads input from an arduino micrphone and performs an FFT analysis. 
Then, it plots the output onto an 8x8 LED matrix. 
The x-axis refers to a range of frequencies, and the y-axis refers to the loudness.
The software can be thought of as an audio spectrum analyzer.
Specific note about our implementation- You will notice that there is never a single
                                        row of lights on at a time. This is because we 
                                        implemented what we call a friend algorithm. 
                                        The code checks to see the length of the height 
                                        of adjacent columns and adjusts the height of its column
                                        accordingly. The reason for this is that the desired 
                                        output turns out more visually appealing.
 
References: We set up up the hardware and some of the FFT analysis using the following instructable tutorial:
               http://www.instructables.com/id/Arduino-Audio-Input/ 
               
This code is free to use, edit, and modify as you please!

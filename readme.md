<h1>Traffic Light Simulator using Queue</h1>

This is a simple real-time traffic light simulation implemented using C programming language, queue and SDL libraries for graphics rendering, the system has a junction with four different lanes with 3 three different sub-lanes for each and one of the sub-lanes modified as a priority lane.

<h2>Project Demonstration:</h2>

![traffic simulator](/traffic_simulator.gif)

<h2>Features</h2>
 
- Real-time automated traffic light simulation

- Priority Lane
    
    The system consists of a priority lane AL2. When the number of vehicles waiting is 10 or more the system automatically lets the traffic in the lane to pass through until there are only 5 left, after which it functions as a normal lane.

<h2>Prerequisites to Run the Project:</h2>

- gcc compiler(or any other C compiler)
- SDL2 library 
- SDL2 TTF Font library

<h2>Run the Project</h2>

After you install all the required materials, build the project using the commands below:

1. First clone the github repository into your local machine, using the command below:

    >`git clone https://github.com/BBP090/dsa-queue-simulator.git`
<br>

2. Compile the receiver in the terminal:

    >`gcc receiver.c -o receiver && ./receiver`

    This will create an executable receiver file, which receives vehicles data.
<br>

3. Compile the simulator to render the graphics and open the SDL window:

    - If you do not have homebrew installed already, do so using the command below:
        >`curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh`
<br>
    - Then compile the simulator in a new terminal:
        >`gcc gcc simulator.c -o simulator -Wall -Wextra -I./include -I/opt homebrew/include $(sdl2-config --cflags --libs) -lSDL2 -lSDL2_ttf -lpthread && ./simulator`
<br>

4. Now, compile the generator, which generates vehicles for the simulation:

    Open a new terminal again and use the command below,
    >`gcc traffic-generator.c -o traffic && ./traffic`
<br>

<h2>References</h2>

- SDL2 official documentation https://wiki.libsdl.org/SDL2/FrontPage
- SDL2 ttf library documentation https://github.com/libsdl-org/SDL_ttf









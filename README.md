# Autonomous Square System
<p>Final project in electronic engineering</p>

<img src='https://i.imgur.com/dPnLJzp.png'/>

---
### System description
<p>
  "Autonomous square system" is a unique system, based on artificial intelligence in deep learning, which combines a traffic circle and a traffic light junction. The     system operates autonomously and only starts working when an <a href='#integration problem'>integration problem</a> arises, and at the rest of the time the square is governed by the rules of the traffic   circle. After each treatment it's provides a detailed report describing the condition of the square and the problem of integration, the way of treatment, the         condition of the square after the treatment and the degree of success. The system can be installed on existing squares and is therefore characterized by great       flexibility that allows the traffic engineer to optimally adapt it to each square specifically through the manager interface.
</p>
<p>
  The system combines two sub-systems:<br>
  <ol>
    <li>
      PC system:<br> 
      Responsible for describing the situation in which the square is at any given moment and locates integration problems by analyzing the information coming from the
      sensors and cameras scattered in the square. In addition, it fills in the treatment reports and is responsible for coordinating the system to the square itself
      and its road conditions, when installing the system, through the <a href='#system administrator'>system administrator</a> interface. The system is written in Python and included about 2,000 lines
      of code divided into a main program (<a href='https://github.com/YakirNissim/Autonomous-Square-System/blob/master/PC%20System/main.py'>main</a>) and two
      libraries (<a href='https://github.com/YakirNissim/Autonomous-Square-System/blob/master/PC%20System/tables_fun.py'>tables_fun<a> and 
      <a href='https://github.com/YakirNissim/Autonomous-Square-System/blob/master/PC%20System/data_transfer.py'>data_transfer<a>).
    <li>
      NUVOTON system:<br> 
      The system is installed in the square itself and is responsible for dealing with integration problems through the management of the traffic lights in the square.
      Finding a unique solution to each problem, testing its effectiveness and finding a new solution if necessary. At the end of the process she also evaluates how
      successful she is in solving the problem. The system is written in C++ and included about 700 lines of code divided into a main program 
      (<a href='https://github.com/YakirNissim/Autonomous-Square-System/blob/master/NUVOTON%20System/main.c'>main<a>) and a secondary file 
      (<a href='https://github.com/YakirNissim/Autonomous-Square-System/blob/master/NUVOTON%20System/Secondary%20file%20containing%20the%20functions%20
      %20video%2C%20itrans%2C%20trans%2C%20InitPIN%2C%20InitTIMER0%2C%20InitINT_GPIO%2C%20InitUART..c'>Secondary file<a>).
    </li>
  </ol>

---
<p id='integration problem'></p>

### **Integration problem**
<p>
  In traffic circles, there is an integration problem, that arises when there is a dominant route in the traffic circle that blocks an entrance path from being
  integrated into it.<br><br>
  Example of an integration problem:<br>
  <p align='center'>
    <img src="https://i.imgur.com/fpgnxVc.jpg"/>
  </p>
  The vehicles marked in red cannot be integrated because of the vehicles traveling on the yellow route.
</p>

---
<p id='system administrator'></p>

### Administrator Interface Flowchart

  <img src='https://i.imgur.com/CzUw8Cn.jpg'/>

---
### Example of a problem report

<img src='https://i.imgur.com/VURCKA5.jpg'/>

---
### Skills I acquired during the project:
<ul>
  <li>
    I’ve learned how to train neural system to object detection.<br>
   The project itself uses trained models for reasons of accuracy and reliability, but during the project I trained a system of neurons to object detection to identify my face and photographed the result (click <a href="#object detection">here<a>).
  </li>
  <li>
    I designed an algorithm to locate issues and to find a unique solution for the problem (i.e. not from a solution stock).
  </li>
</ul>

---
<p id='object detection'></p>

### Results of training systems of neurons to object detection
  
  https://user-images.githubusercontent.com/101890349/160665039-59339fe3-67db-4b54-af76-8058fe94cc88.mp4
  

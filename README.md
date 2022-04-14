# Overview

This is a documentation for 6.08 design exercise trivia game. This consists three different parts: overall design flow, server file(s) explanation, and state machines/any other details being implemented in src.ino.

The demonstration video is linked below
[demonstration video](https://youtu.be/Zf1zj-SdbLM)

## Resources

If you have any images, place them in a folder called `writeup_resources` in the top level of the folder. Then you can reference the images by simply using a relative path like I'm doing below:

![An example test image](./writeup_resources/beaver.jpg)

If you want to include code snippets it is easy:

```cpp

void int_adder(int x, int y){
    return x+y; //cool
}
```

A `README.md` is a markdown file which means it is written in Markdown, which is a super-lightweight way to create content that is widely used to document projects. It is a good skill to know.  If you need help learning markdown, there are tons of good resources such as [this](https://www.markdownguide.org/getting-started/).

# 1. Overall Design Flow
## NOTE: You can change user name by changing `USER[]` in the code. 
The LCD display mainly consists of four screens: 
1. opening screen, 
which display current user name, historical statistics (ie.total score, total number of correct/wrong answers), and instruction to press `button 38` to start a new game.
2. gaming screen, which transits from (1) by pressting `button 38`. It displays a local game score, number of correc/wrong answers (all starts with 0s), question, and instruction to press `button 39` for answering True, `button 45` for answering False, and `button 38` to terminate the game and recalculate the historical total score. Note that pressing `button 38` triggers a POST request to `https://608dev-2.net/sandbox/sc/xieyi/trivia_score.py` (more details in section 2).
3. By answering a question correctly/incorrectly, a corresponding screen saying "CORRECT" or "INCORRECT" will pop up for 400ms, then bring you back to screen (2). 
4. The game cycles between 2 and 3 until the user presses `button 38` to terminate the game. Note, my design choice is to only POST when a game is terminated because a user can spam True/False button and it does not make too much sense to POST every second.

# 2. Server File(s)
## 1. travia_request.py (https://608dev-2.net/sandbox/sc/xieyi/travia_request.py)
-> This server script simply handles a request to trivia API whenever it's called. It also processes the response from API and converting unicode apostrophes and quotes. It returns a JSON dictionary consisting of a question and a corresponding answer.
-> To avoid repetitive questions, I randomly choose category from 9 to 12 in this file.

## 2. trivia_score.py (https://608dev-2.net/sandbox/sc/xieyi/trivia_score.py)
-> a) When there's a GET request with `scoreboard = True`, it displays a scoreboard, ranking from the highest to the lowest scores of all users in browser, which can be viewed here: (`https://608dev-2.net/sandbox/sc/xieyi/trivia_score.py?scoreboard=True`)
-> b) When there's a GET request with `score = True` and a `user` argument, the script will look up in the database whether this user is in the database. If so, it will return the user's historical score (score, correct_num, wrong_num); else, it will return all 0s indicating there's a new user, and create another entry in the database.
-> c) When there's a POST request, with `score`, `correct_num`, and `wrong_num` arguments, it will create a new row in the database if it is a new user, else it will only increment the corresponding user's existing entries.
-> d) To make an HTML list, I have an extra function named `ulify`.
```
def ulify(elements):
    string = "<h2>Scoreboard</h2>\n"
    # string += "user\tscore\tcorrect number\twrong number\n"
    string += "<ol>\n"
    for x in elements:
        string += "<li>" + "User: "+str(x[0]) + ", Score:"+str(x[1])+", Correct:" + \
            str(x[2])+", Wrong: "+str(x[3]) + "</li>\n"
    string += "</ol>"
    return string
```
It essentially hard codes html tags into strings.


# 3. State Machines/Other Details
A state machine is implemented to handle transitions and make the game replayable. 
It consists of 8 states, with 5 waiting states to handle latency and button press.

In `START` state, it initalizes a GET request discussed in 2.2b to get historical stat with current user name, else it initalizes to 0s. It shows screen (1). On press button 38, it will go through `WAIT_1` really quick and on release button 38 it goes into `QUESTION` state.

In `QUESTION` state, it sends a GET request once as discussed in 2.1. We process the JSON dictionary and display local stat, question, and instruction (screen (2)).
-> By pressing button 39 to choose the answer is True, it goes through `WAIT_2` to handle button release, then goes in `CHECK_ANS`.
-> By pressing button 45 to choose the answer is False, it goes through `WAIT_3` to handle button release, then goes in `CHECK_ANS`.
-> By pressing button 38 to terminate the game and calculate the score, it goes through `WAIT_4` to handle button release, sends a POST request once as discussed in 2.2c, and on release goes back to screen(1). 

In `CHECK_ANS`, it compares if user answer is the same as the trivia answer. If so, it increments both `score` and `num_correct` by 1, and shows screen (3). Else, it decrements both `score` and `num_incorrect` by 1, and shows screen (3). Then, it resets and goes through `WAIT_QUESTION` -> `QUESTION` (back to screen (1)).



import sqlite3
import datetime
visits_db = '/var/jail/home/xieyi/trivia_score2.db'


def request_handler(request):
    if request["method"] == "GET" and 'scoreboard' in request['args']:
        if request['values']['scoreboard']:
            # When we do a GET request to your server-side script with the query argument "scoreboard=True",
            # an html list should be returned with the history of all scores on your system sorted highest to lowest.
            conn = sqlite3.connect(visits_db)
            c = conn.cursor()  # move cursor into database (allows us to execute commands)
            things = c.execute(
                '''SELECT DISTINCT * FROM trivia_table ORDER BY score DESC;''').fetchall()
            outs = ulify(things)
            conn.commit()  # commit commands
            conn.close()  # close connection to database
            return outs
    elif request["method"] == "GET" and request['values']['score'] and "user" in request["args"]:
        # connect to that database (will create if it doesn't already exist)
        user = request['values']['user']
        conn = sqlite3.connect(visits_db)
        c = conn.cursor()  # move cursor into database (allows us to execute commands)
        # select user if exist
        things = c.execute(
            '''SELECT * FROM trivia_table WHERE user = ?''', (user,)).fetchall()
        if things:
            return {'score': things[0][1], "correct_num": things[0][2], "wrong_num": things[0][3]}
        else:
            return {'score': 0, "correct_num": 0, "wrong_num": 0}

    elif request["method"] == "POST":
        score = int(request['form']['score'])
        correct_num = int(request['form']['correct_num'])
        wrong_num = int(request['form']['wrong_num'])
        user = request['form']['user']

        # connect to that database (will create if it doesn't already exist)
        conn = sqlite3.connect(visits_db)
        c = conn.cursor()  # move cursor into database (allows us to execute commands)
        outs = ""
        c.execute('''CREATE TABLE IF NOT EXISTS trivia_table (user text,score integer, correct_num integer, wrong_num integer);''')  # run a CREATE TABLE command
        c.execute('''INSERT into trivia_table VALUES (?,?,?,?) ON CONFLICT(user) DO UPDATE SET score=score+?, correct_num=correct_num+?, wrong_num=wrong_num+? WHERE user=?;''',
                  (user, score, correct_num, wrong_num, score, correct_num, wrong_num, user))
        # UPDATE t1 SET c=c+1 WHERE a=1 OR b=2 LIMIT 1;
        # INSERT INTO table (id, name, age) VALUES(1, "A", 19) ON DUPLICATE KEY UPDATE name="A", age=19
        # c.execute('''INSERT into trivia_table VALUES (?,?,?,?);''',user, score, correct_num, wrong_num))
        things = c.execute(
            '''SELECT DISTINCT * FROM trivia_table ORDER BY score DESC;''').fetchall()
        outs = "Scoreboard\n"
        for x in things:
            outs += str(x[0])+"\t\t"+str(x[1])+"\t\t" + \
                str(x[2])+"\t\t"+str(x[3])+"\n"
        outs = ulify(things)
        conn.commit()  # commit commands
        conn.close()  # close connection to database
        return outs


def ulify(elements):
    string = "<h2>Scoreboard</h2>\n"
    # string += "user\tscore\tcorrect number\twrong number\n"
    string += "<ol>\n"
    for x in elements:
        string += "<li>" + "User: "+str(x[0]) + ", Score:"+str(x[1])+", Correct:" + \
            str(x[2])+", Wrong: "+str(x[3]) + "</li>\n"
    string += "</ol>"
    return string

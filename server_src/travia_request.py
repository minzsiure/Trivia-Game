#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Wed Mar  2 20:26:01 2022

@author: yixie
"""

import requests
import json
import pprint  # prettyprint in python
import random

# r = requests.get('https://opentdb.com/api.php?amount=1&category=9&type=boolean')
# response = r.json()
# pprint.pprint(response)

# qs = response['results'][0]['question']
# print(qs)
# qs = qs.replace("&quot;", "\"")
# qs = qs.replace("&#039;", "\'")
# ans = response['results'][0]['correct_answer']
# print(qs)


def request_handler(request):
    category = random.randint(9, 13)
    url = "https://opentdb.com/api.php?amount=1&category=" + \
        str(category)+"&type=boolean"
    # 'https://opentdb.com/api.php?amount=1&category=9&type=boolean'
    r = requests.get(url)
    response = r.json()
    pprint.pprint(response)

    qs = response['results'][0]['question']

    qs = qs.replace("&quot;", "\"")
    qs = qs.replace("&#039;", "\'")
    ans = response['results'][0]['correct_answer']

    return {'question': qs, 'answer': ans}

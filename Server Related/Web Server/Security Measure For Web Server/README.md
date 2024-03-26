1) Never trust client input
        https://stackoverflow.com/questions/2334863/does-html-encoding-prevent-xss-security-exploits
        https://stackoverflow.com/questions/1976007/what-characters-are-forbidden-in-windows-and-linux-directory-names

3) Abnormal/altered request is possible, don't trust every request
    https://stackoverflow.com/questions/12233406/preventing-session-hijacking

4) The server side is the safest and most vulnerable. (To make it the safest, you need to patch all the vulnerabilities)

5) I have not dealt with PHP for a long time, if you use it, do search for its vulnerability
    https://www.netsolutions.com/insights/severe-php-vulnerabilities-how-to-fix-them/

Interesting story:
StackOverflow 2019 Security Incident - https://stackoverflow.blog/2021/01/25/a-deeper-dive-into-our-may-2019-security-incident/

import subprocess
import datetime
from datetime import timedelta

#s = subprocess.call(["gammu", "getussd", "*101#"])
#cmd = "gammu getussd \"*101#\""
temp = subprocess.Popen(["gammu", "getussd", "*101#"], stdout = subprocess.PIPE)
s = str(temp.communicate()) 

arrs = s.split()
num = []
for i in arrs:
	if "TKC" in i:
		tkc = int(filter(str.isdigit, i))
		num.append(tkc)
	if i.isdigit():
		num.append(int(i))
	if "/20" in i:
		expDate = datetime.datetime.strptime(i[:-1], '%d/%m/%Y').date()

today = datetime.datetime.now().date()

if num[0] < 5000:
	print("Warning! TKC less than 5000 VND")
if (today + timedelta(days=30)) >= expDate:
	print("Warning! Expire date: ".format(expDate)) 


#xcal.py
#
#Script for calendar GUI utility
#Created: March 5, 2016
#Last Updated: April 7, 2016
#Author:Ajeesh Srijeyarajah(0884990
#Contact: asrijeya@mail.uoguelph.ca

#!/usr/bin/python3
from tkinter import *
from tkinter.ttk import *
from tkinter.filedialog import askopenfilename
from tkinter.filedialog import asksaveasfilename
from tkinter.messagebox import *
import tkinter as tk
from tkinter.scrolledtext import ScrolledText
import os
import subprocess
from subprocess import *

import Cal
import mysql.connector
from mysql.connector import Error
import sys
import getpass

global currentFile
currentFile = "None"

unsaved = False

def quitProg(cnx):
    if askyesno("Exit", "Are you sure you want to exit?"):
        os.system("rm -f fvp.ics")
        cnx.close() 
        mw.quit();
def datemskPopup(r):
    if askyesno("Datemsk", "DATEMSK has not been set, would you like to set it now?"):
        setDatemsk(r)


def setDatemsk(r):
    file_options = {}
    file_options['defaultextension'] = ''
    file_options['filetypes'] = ''
    file_options['initialdir'] = ''
    file_options['initialfile'] = ''
    file_options['parent'] = r
    file_options['title'] = 'Set Datemsk'
    fileName = askopenfilename(**file_options)
    if fileName:
        os.environ['DATEMSK'] = fileName
    
def openFile(r,fvp,log,dbmenu):
    file_options = {}
    file_options['defaultextension'] = '.ics'
    file_options['filetypes'] = ''
    file_options['initialdir'] = ''
    file_options['initialfile'] = ''
    file_options['parent'] = r
    file_options['title'] = 'Open File'
    global currentFile
    global unsaved
    if unsaved == True:
        if askyesno("Replace?", "File with unsaved changes already open, ok to discard changes?"):
    
            ret = askopenfilename(**file_options)
            if ret:
                try:
                    command = "./caltool -info"
                    infile = open(ret)
                    r.title("XCal-None")
                    currentFile = "None"
                    fvp.delete(*fvp.get_children())
                    output = subprocess.check_output(command, stdin = infile,stderr = STDOUT,shell=True)
                except CalledProcessError as exc:
                    log.insert(END, exc.output)
                else:
                    log.insert(END, output)
                    r.title("XCal-" + ret)
                    currentFile = ret
                    result = []
                    status = Cal.readFile(ret,result)
                    infile.close()
                    if status == "OK":
                        pcal = result[0]
                        for i in range(1, len(result)):
                            fvp.insert('','end', text = str(i), values = result[i])
                        Cal.freeFile(pcal)
                        fvpFile = open("fvp.ics", 'w')
                        inFh = open(ret, 'r')
                        for line in inFh:
                            fvpFile.write(line)
                        unsaved = False
                        dbmenu.entryconfigure('Store All', state='active')
                        dbmenu.entryconfigure('Store Selected', state='active')
                    else:
                        log.insert(END, status)
    else:        
        ret = askopenfilename(**file_options)
        if ret:
            try:
                command = "./caltool -info"
                infile = open(ret)
                r.title("XCal-None")
                currentFile = "None"
                fvp.delete(*fvp.get_children())
                output = subprocess.check_output(command, stdin = infile,stderr = STDOUT,shell=True)
            except CalledProcessError as exc:
                log.insert(END, exc.output)
            else:
                log.insert(END, output)
                r.title("XCal-" + ret)
                currentFile = ret
                result = []
                status = Cal.readFile(ret,result)
                infile.close()
                if status == "OK":
                    pcal = result[0]
                    for i in range(1, len(result)):
                        fvp.insert('','end', text = str(i), values = result[i])
                    Cal.freeFile(pcal)
                    fvpFile = open("fvp.ics", 'w')
                    inFh = open(ret, 'r')
                    for line in inFh:
                        fvpFile.write(line)
                    unsaved = False
                    dbmenu.entryconfigure('Store All', state='active')
                    dbmenu.entryconfigure('Store Selected', state='active')
                else:
                    log.insert(END, status)

def populateFvp(fvp):
    fvp.delete(*fvp.get_children())
    resultList = []
    fileName = "fvp.ics"
    status = Cal.readFile(fileName, resultList)
    for i in range(1, len(resultList)):
        fvp.insert('','end', text = str(i), values = resultList[i])

    return fvp
def save(log,r):
    global currentFile
    global unsaved
    if unsaved == False:
        log.insert(END, "Nothing to Save\n")
    else:
        outfile = open(currentFile, 'w')
        infile = open("fvp.ics", 'r')
        numLines = 0
        for line in infile:
            outfile.write(line)
            numLines = numLines + 1

        output = "Saved " + str(numLines) + " lines\n"
        log.insert(END, output)
        unsaved = False
        r.title("XCal-" + currentFile)

 
def saveAs(r,log):
    global currentFile
    global unsaved
    file_options = {}
    file_options['filetypes'] = [('ics files', '.ics')]
    file_options['parent'] = r
    file_options['initialdir'] = ''
    file_options['title'] = 'Save As'
    ret = asksaveasfilename(**file_options)
    if ret:
        fh = open("fvp.ics",'r')
        outfile = open(ret, 'w')
        numLines = 0
        for line in fh:
            outfile.write(line)
            numLines = numLines + 1

        r.title("XCal-" + ret)
        output = "Saved " + str(numLines) + " lines to " + ret + "\n"
        log.insert(END, output)
        currentFile = ret
        unsaved = False

def filterCal(r,fvp, log):
    toplevel = Toplevel()
    toplevel.title("Filter")
    type = StringVar()

    eventRadio = Radiobutton(toplevel, text = "Event",variable = type, value = "Event", command=lambda:filterBtn.config(state=ACTIVE)).grid(row=1,column=1)
   
    fromlbl=Label(toplevel, text = "From")
    fromlbl.grid(row=0,column=2)
 
    fromDate = Entry(toplevel)
    fromDate.grid(row=1,column=2)

    tolbl = Label(toplevel, text = "To")
    tolbl.grid(row=0, column=3)
    toDate = Entry(toplevel)
    toDate.grid(padx=(5,0),row=1,column=3)

    todoRadio = Radiobutton(toplevel, text = "Todo",variable = type, value = "Todo", command=lambda:filterBtn.config(state=ACTIVE)).grid(row=2,column=1) 
 

    filterBtn = Button(toplevel, text = "Filter", command=lambda:confirmFilter(type.get(),fvp, fromDate, toDate,log,r))
    filterBtn.grid(row=3,column=2)
    filterBtn.config(state=DISABLED)
    

    cancelBtn = Button(toplevel, text = "Cancel", command=lambda: toplevel.withdraw())
    cancelBtn.grid(row=3,column=3)
    toplevel.transient(r)
    r.grab_set()
    

    

def confirmFilter(type,fvp,fromDate,toDate,log,r):
    infile = open("fvp.ics", 'r+')
    errfile = open("err.txt", 'w')
    global unsaved
    if type == "Todo":
        try:
            command = "./caltool -filter t"
            output = subprocess.check_output(command, stdin = infile,stderr = STDOUT,shell = True)
        except CalledProcessError as exc:
            log.insert(END, exc.output)
        else:
            data = infile.read()
            infile.seek(0)
            infile.write(output.decode("utf-8"))
            infile.truncate()
            infile.close()
            fvp = populateFvp(fvp)
            unsaved = True
            r.title("XCal-" + currentFile + "*")

    elif type == "Event":
       if fromDate.get() and toDate.get():
            date1 = fromDate.get()
            date2 = toDate.get()
            if date1 != "today":
                date1 = "\"" + date1 + "\""
            if date2 != "today":
                date2 = "\"" + date2 + "\""

            try:
                command = "./caltool -filter e from " + date1 + " to " + date2
                output = subprocess.check_output(command, stdin = infile, stderr = STDOUT, shell = True)
            except CalledProcessError as exc:
                log.insert(END, exc.output)
            else:
                data = infile.read()
                infile.seek(0)
                infile.write(output.decode("utf-8"))
                infile.truncate()
                infile.close()
                fvp = populateFvp(fvp)
                unsaved = True
                r.title("XCal-" + currentFile + "*")
       elif fromDate.get() and not toDate.get():
            date1 = fromDate.get()

            if date1 != "today":
                date1 = "\"" + date1 + "\""
            
            try:
                command = "./caltool -filter e from " + date1
                output = subprocess.check_output(command, stdin = infile, stderr = STDOUT, shell = True)
            except CalledProcessError as exc:
                log.insert(END, exc.output)
            else:
                data = infile.read()
                infile.seek(0)
                infile.write(output.decode("utf-8"))
                infile.truncate()
                infile.close()
                fvp = populateFvp(fvp)
                unsaved = True
                r.title("XCal-" + currentFile + "*")
       elif not fromDate.get() and toDate.get():
            
            date1 = toDate.get()

            if date1 != "today":
                date1 = "\"" + date1 + "\""
            
            try:
                command = "./caltool -filter e to " + date1
                output = subprocess.check_output(command, stdin = infile, stderr = STDOUT, shell = True)
            except CalledProcessError as exc:
                log.insert(END, exc.output)
            else:
                data = infile.read()
                infile.seek(0)
                infile.write(output.decode("utf-8"))
                infile.truncate()
                infile.close()
                fvp = populateFvp(fvp)
                unsaved = True
                r.title("XCal-" + currentFile + "*")
           
       else:
            try:
                command = "./caltool -filter e"
                output = subprocess.check_output(command, stdin = infile, stderr = STDOUT, shell = True)
            except CalledProcessError as exc:
                log.insert(END, exc.output)
            else:
                data = infile.read()
                infile.seek(0)
                infile.write(output.decode("utf-8"))
                infile.truncate()
                infile.close()
                fvp = populateFvp(fvp)
                unsaved = True
                r.title("XCal-" + currentFile + "*")
def combineCal(log,fvp,r):
    global unsaved 
    file_options = {}
    file_options['defaultextension'] = '.ics'
    file_options['filetypes'] = ''
    file_options['initialdir'] = ''
    file_options['initialfile'] = ''
    file_options['parent'] = r
    file_options['title'] = 'Open File'
    ret = askopenfilename(**file_options)
    if ret:
        infile = open("fvp.ics", 'r+')
        try:
            command = "./caltool -combine " + ret
            output = subprocess.check_output(command, stdin = infile, stderr = STDOUT, shell = True)
        except CalledProcessError as exc:
            log.insert(END, exc.output)
        else:
            date = infile.read()
            infile.seek(0)
            infile.write(output.decode("utf-8")) 
            infile.truncate()
            infile.close()
            fvp = populateFvp(fvp) 
            unsaved = True
            r.title("XCal-" + currentFile + "*")
def todoList(r,fvp):
    toplevel = Toplevel()
    toplevel.title("Todo List")
    
    vsb = Scrollbar(toplevel, orient="vertical")
    text = Text(toplevel, width = 40, height = 20, yscrollcommand = vsb.set)
    vsb.config(command = text.yview)
    vsb.pack(side="right", fill="y")
    text.pack(side="left", fill="both", expand=True)

    for child in fvp.get_children():
        list = fvp.item(child)['values']
        if(list[0] == 'VTODO'):
            cb = Checkbutton(toplevel, text = list[3])
            text.window_create("end", window=cb)
            text.insert("end","\n")

    doneBtn = Button(toplevel, text="Done", command=lambda:toplevel.destroy())
    doneBtn.pack()
    cancelBtn = Button(toplevel, text="Cancel", command=lambda:toplevel.destroy())
    cancelBtn.pack()
    toplevel.transient(r)
    r.grab_set()

def extractXProp(log):
    infile = open("fvp.ics", 'r')
    if infile:
        try:
            command = "./caltool -extract x "
            output = subprocess.check_output(command, stdin = infile, stderr = STDOUT, shell = True)
        except CalledProcessError as exc:
            log.insert(END, exc.output)
        else:
            log.insert(END, output)

def extractEvent(log):
    infile = open("fvp.ics", 'r')
    if infile:
        try:
            command = "./caltool -extract e"
            output = subprocess.check_output(command, stdin = infile, stderr = STDOUT, shell = True)
        except CalledProcessError as exc:
            log.insert(END, exc.output)
        else:
            log.insert(END, output)

def showSelectedComp(log):
    log.insert(END, "Show Selected - Clicked\n")
def undo(log):
    if askyesno("Undo", "Warning - All TODO components removed since last save will be restored. Is this okay?"):
        log.insert(END,"undo stub-yes")
    else:
        log.insert(END,"undo stud-no")

def confirmQuery(resultLog, txtOrg, txtLoc, txtPriority, txtMy, txtCustom, mode, cnx):
    cursor = cnx.cursor()
    
    if mode == "org":
        if not txtOrg.get():
            return
        orgName = txtOrg.get()

        query = "SELECT org_id FROM ORGANIZER where name = \"%s\";" % orgName
        cursor.execute(query)
        try:
            orgID = int(cursor.fetchone()[0])
        except TypeError as e:
            resultLog.insert(END, "None\n")
            resultLog.insert(END,"---------------------\n")
            return
        query = "SELECT summary FROM EVENT where organizer = \"%d\";" % orgID
        cursor.execute(query)
        result = cursor.fetchall()        
        for line in result:
            for entry in line:
                resultLog.insert(END,entry)
                resultLog.insert(END,"\n")

        query = "SELECT summary FROM TODO where organizer = \"%d\";" %orgID
        cursor.execute(query)
        result = cursor.fetchall()
        for line in result:
            for entry in line:
                resultLog.insert(END,entry)
                resultLog.insert(END,"\n")

        resultLog.insert(END,"---------------------\n")

    elif mode == "loc":
        if not txtLoc.get():
            return

        location = txtLoc.get()
        
        query = "SELECT location, COUNT(*) FROM EVENT where location = \"%s\";" % location
        try:
            cursor.execute(query)
        
            result = int(cursor.fetchone()[1])
        

            output = "%d events take place in location %s\n" % (result, location)
            resultLog.insert(END,output)
        except Error as e:
            resultLog.insert(END,e)
            resultLog.insert(END, "\n")
        resultLog.insert(END,"---------------------\n")


    elif mode == "start":
        query = "SELECT summary FROM EVENT ORDER BY(start_time);"
        try:
            cursor.execute(query)
            result = cursor.fetchall()
            for line in result:
                for entry in line:
                    resultLog.insert(END, entry)
                    resultLog.insert(END, "\n")
        except Error as e:
            resultLog.insert(END,e)
            resultLog.insert(END,"\n")


        resultLog.insert(END,"---------------------\n")
    elif mode == "priority":
        if not txtPriority.get():
            return

        priority = int(txtPriority.get())
        
        query = "SELECT summary FROM TODO where priority = \"%d\";" % priority
        try:
            cursor.execute(query)
            result = cursor.fetchall()
            for line in result:
                for entry in line:
                    resultLog.insert(END, entry)
                    resultLog.insert(END, "\n")

        except Error as e:
            resultLog.insert(END, e)
            resultLog.insert(END, "\n")


        resultLog.insert(END,"---------------------\n")
            
    elif mode == "my":
        if not txtMy.get():
            return


        orgName = txtMy.get()
        query = "SELECT org_id FROM ORGANIZER where name = \"%s\";" % orgName
        cursor.execute(query)
        try:
            orgID = int(cursor.fetchone()[0])
        except TypeError as e:
            resultLog.insert(END, "None\n")
            resultLog.insert(END,"---------------------\n")
            return
        
        query = "SELECT summary, MIN(priority) FROM TODO where organizer = \"%d\";" %orgID
        
        try:
            cursor.execute(query)
            result = cursor.fetchall()
            for line in result:
                resultLog.insert(END, line[0])
                resultLog.insert(END, "\n")

        except Error as e:
            resultLog.insert(END, e)
            resultLog.insert(END, "\n")

        
        resultLog.insert(END,"---------------------\n")
    elif mode == "custom":
        if not txtCustom.get():
            return


        query = txtCustom.get()

        try:
            cursor.execute(query)
            for line in cursor:
                resultLog.insert(END,line)
                resultLog.insert(END,"\n")

        except Error as e:
            resultLog.insert(END, e)
            resultLog.insert(END, "\n")

        resultLog.insert(END,"---------------------\n")
    cursor.close()

def helpWindow(cnx):
    toplevel = Toplevel()
    toplevel.title("Help")
    
    field = ScrolledText(toplevel, width = 80, height = 10)
    field.pack()
    cursor = cnx.cursor()

    
    output = "Table ORANIZER: Columns\n orgId|name|contact\n\n"
    field.insert(END, output)

    output = "Table EVENT: Columns\n event_id|summary|startTime|location|organizer\n\n"
    field.insert(END, output)
    
    output = "Table TODO: Columns\n todo_id|summary|priority|organizer\n\n"
    field.insert(END, output)
def queryWindow(r,cnx):
    toplevel = Toplevel()
    toplevel.title("Query")

    mode = StringVar()

    
    orgRadio = Radiobutton(toplevel, text = "Display the items of organizer",variable = mode, value = "org").grid(row=0,column = 0,sticky='w')
    lblOrg = Label(toplevel, text = "Organizer:").grid(row=0,column = 1)
    txtOrg = Entry(toplevel)
    txtOrg.grid(row=0, column=2)

    locRadio = Radiobutton(toplevel, text = "How many events take place in",variable = mode, value = "loc").grid(row=1,column=0,sticky='w')
    lblLoc = Label(toplevel, text = "Location:").grid(row=1,column=1)
    txtLoc = Entry(toplevel)
    txtLoc.grid(row=1,column=2)

    startRadio = Radiobutton(toplevel, text = "Sort Events From Earliest to Latest",variable = mode, value = "start").grid(row=2,column=0,sticky='w')

    priorityRadio = Radiobutton(toplevel, text = "Find All To-do With Priority N",variable = mode, value = "priority").grid(row=3,column=0,sticky='w')
    lblPriority = Label(toplevel, text = "Priority:").grid(row=3,column=1)
    txtPriority = Entry(toplevel)
    txtPriority.grid(row=3,column=2)
    
    myRadio = Radiobutton(toplevel, text = "Find Highest Priority To-Do by Organizer: ",variable = mode, value = "my").grid(row=4,column=0,sticky='w')
    lblMy = Label(toplevel, text = "Organizer Name:").grid(row=4,column=1)
    txtMy = Entry(toplevel)
    txtMy.grid(row=4,column=2)

    customRadio = Radiobutton(toplevel, text = "Custom (ad hoc) Query",variable = mode, value = "custom").grid(row=5,column=0,sticky='w')
    lblCustom = Label(toplevel, text = "Enter your query:").grid(row=5,column=1)
    txtCustom = Entry(toplevel)
    txtCustom.insert(END, 'SELECT')
    txtCustom.grid(row=5,column=2)

    submitBtn = Button(toplevel, text = "Submit", command=lambda:confirmQuery(resultLog, txtOrg, txtLoc, txtPriority, txtMy, txtCustom, mode.get(), cnx))
    submitBtn.grid(row = 6, column = 0)

    helpBtn = Button(toplevel, text = "Help", command=lambda:helpWindow(cnx))
    helpBtn.grid(row = 6, column = 1)

    resultLog = ScrolledText(toplevel, width = 80, height = 10)
    resultLog.grid(row=7, column = 1, sticky=(W,S,E))

    clearButton = Button(toplevel, text="CLEAR", command=lambda:resultLog.delete(1.0,END))
    clearButton.grid(row=8, column = 1)

    toplevel.grid_columnconfigure(0, weight = 1)
    toplevel.grid_rowconfigure(0, weight = 1)

    toplevel.transient(r)
    toplevel.grab_set()

def storeAll(cnx,log):
    list = []

    status = Cal.getFVP(list)

    for i in range(len(list)):
        if len(list[i]) == 5:
            summary = list[i][0]
            startTime = list[i][1]
            location = list[i][2]
            orgName = list[i][3]
            orgContact = list[i][4]
            
            
            if orgContact != 'NULL':
                tempOrg = orgContact.split(':')
                orgContact = ""
                orgContact = tempOrg[1] + ":" + tempOrg[2]
           

            cursor = cnx.cursor()
            if orgName != 'NULL' and summary != 'NULL' and startTime != 'NULL':
                if location != 'NULL':
                    location = "'" + location + "'"

                query = "SELECT COUNT(*) FROM ORGANIZER where name = '%s';" % orgName 
                cursor.execute(query)
                result = int(cursor.fetchone()[0])
                if(result == 0):
                    query = "INSERT INTO ORGANIZER (name, contact) VALUES ('%s','%s');" % (orgName, orgContact)
                    cursor.execute(query)
                    cnx.commit()
                
                query = "SELECT org_id from ORGANIZER where name = '%s';" % orgName
                cursor.execute(query)
                orgID = int(cursor.fetchone()[0])

                query = "SELECT COUNT(*) FROM EVENT where summary = '%s' AND start_time = '%s';" % (summary, startTime)
                cursor.execute(query)
                ret = int(cursor.fetchone()[0])
                
                if ret == 0:
                    query = "INSERT INTO EVENT (summary, start_time, location, organizer) VALUES ('%s', '%s', %s, %d);" %(summary, startTime, location, orgID)
                    cursor.execute(query)
                    cnx.commit()

            elif orgName == 'NULL' and summary != 'NULL' and startTime != 'NULL':
               
                query = "SELECT COUNT(*) from EVENT where summary = \"%s\" AND start_time = '%s';" % (summary, startTime)
                cursor.execute(query)
                ret = int(cursor.fetchone()[0])
                
                if ret == 0:
                    query = "INSERT INTO EVENT (summary, start_time, location, organizer) VALUES ('%s', '%s', \"%s\", NULL);" %(summary, startTime, location)
                    cursor.execute(query)
                    cnx.commit()

            cursor.close()
        elif len(list[i]) == 4:
            summary = list[i][0]
            priority = int(list[i][1])
            orgName = list[i][2]
            orgContact = list[i][3]

            if orgContact != 'NULL':
                tempOrg = orgContact.split(':')
                orgContact = ""
                orgContact = orgContact + tempOrg[1] + ":" + tempOrg[2]

            cursor = cnx.cursor()
            if orgName != 'NULL' and priority != 1000:
                
                query = "SELECT COUNT(*) FROM ORGANIZER where name = \"%s\";" % orgName
                cursor.execute(query)
                result = int(cursor.fetchone()[0])

                if result == 0:
                    query = "INSERT INTO ORGANIZER (name, contact) VALUE (\"%s\", \"%s\");" % (orgName, orgContact)
                    cursor.execute(query)
                    cnx.commit()
                
                query = "SELECT org_id FROM ORGANIZER where name = \"%s\";" % orgName
                cursor.execute(query)
                orgID = int(cursor.fetchone()[0])

                query = "SELECT COUNT(*) FROM TODO WHERE summary = \"%s\";" % summary
                cursor.execute(query)
                ret = int(cursor.fetchone()[0])

                if ret == 0:
                    query = "INSERT INTO TODO (summary, priority, organizer) VALUE (\"%s\", \"%d\", \"%d\");" % (summary, priority, orgID)
                    cursor.execute(query)
                    cnx.commit()
            elif orgName == 'NULL' and priority != 1000:
                query = "SELECT COUNT(*) FROM TODO WHERE summary = \"%s\";" % summary
                cursor.execute(query)
                ret = int(cursor.fetchone()[0])
                
                if ret == 0:
                    query = "INSERT INTO TODO (summary,priority, organizer) VALUE (\"%s\", \"%d\", NULL);" % (summary, priority)

            cursor.close()
    getStatus(cnx,log)
def clearTables(cnx,log):
    cursor = cnx.cursor()
    query = "DELETE FROM ORGANIZER"

    try:
        cursor.execute(query)
    except Error as e:
        log.insert(END, e)

    query = "DELETE FROM EVENT"
    
    try:
        cursor.execute(query)
    except Error as e:
        log.insert(END, e)

    query = "DELETE FROM TODO"
    
    try:
        cursor.execute(query)
    except Error as e:
        log.insert(END,e)

    getStatus(cnx,log)
    cursor.close()
def getStatus(cnx,log):
    cursor = cnx.cursor()

    query = "SELECT COUNT(*) FROM ORGANIZER"
    cursor.execute(query)
    numOrgs = cursor.fetchone()[0]

    query = "SELECT COUNT(*) FROM EVENT"
    cursor.execute(query)
    numEvent = cursor.fetchone()[0]

    query = "SELECT COUNT(*) FROM TODO"
    cursor.execute(query)
    numTodo = cursor.fetchone()[0]

    output = "Database has %d organizers, %d events, %d to-do items.\n" % (numOrgs, numEvent, numTodo)
    log.insert(END,output)   
def mainWindowMenu(r,fvp,log,cnx):
    m = Menu(r)    
    filemenu = Menu(m, tearoff=0)
    filemenu.add("command", label="Open", command=lambda:openFile(r,fvp,log,dbmenu), accelerator="Ctrl+O")
    filemenu.add("command", label="Save", command=lambda:save(log,r), accelerator="Ctrl+S")
    filemenu.add("command", label="Save As", command=lambda:saveAs(r,log))
    filemenu.add("command", label="Combine", command=lambda:combineCal(log,fvp,r))
    filemenu.add("command", label="Filter", command=lambda:filterCal(r,fvp,log))
    filemenu.add("command", label="Exit", command=lambda:quitProg(cnx), accelerator="Ctrl+X")

    todomenu = Menu(m, tearoff=0)
    todomenu.add("command", label="TodoList", command=lambda:todoList(r,fvp), accelerator="Ctrl+T")
    todomenu.add("command", label="Undo", command=lambda:undo(log), accelerator="Ctrl+Z")

    helpmenu = Menu(m, tearoff=0)
    helpmenu.add("command", label="Date Mask", command=lambda:setDatemsk(r))
    helpmenu.add("command", label="About X-Cal", command=aboutWindow)

    dbmenu = Menu(m, tearoff=0)
    dbmenu.add("command", label="Store All", command=lambda:storeAll(cnx,log))
    dbmenu.add("command", label="Store Selected", command=lambda:quitProg(cnx))
    dbmenu.add("command", label="Clear", command=lambda:clearTables(cnx,log))
    dbmenu.add("command", label="Status",command=lambda:getStatus(cnx,log))
    dbmenu.add("command", label="Query", command=lambda:queryWindow(r,cnx))


    m.add("cascade",menu=filemenu, label="File")
    m.add("cascade",menu=todomenu, label="Todo")
    m.add("cascade",menu=helpmenu, label="Help")
    m.add("cascade",menu=dbmenu, label="Database")
    
    dbmenu.entryconfigure('Store All', state='disabled')
    dbmenu.entryconfigure('Store Selected', state='disabled')
    r.bind_all("<Control-o>",lambda x :openFile(r,fvp,log,dbmenu))
    r.bind_all("<Control-s>",lambda x :save(log,r))
    r.bind_all("<Control-x>",lambda x :quitProg(cnx))
    r.bind_all("<Control-t>",lambda x :todoList(r,fvp))
    r.bind_all("<Control-z>",lambda x :undo(log))  
    return m

def aboutWindow():
    dialogue = ("Welcome to XCal\n "
                "Author: Ajeesh Srijeyarajah\n "
                "Compatible With iCalendar V 2.0\n");
    showinfo("About", dialogue)

def _len_(t):
    return len(t)
    
def buildMainWindow(cnx):
    root = Tk()
    root.title("XCal-" + currentFile)
    
    root.protocol('WM_DELETE_WINDOW', quitProg) 
 
    fvp = Treeview(root)
    fvpScroll = Scrollbar(root, orient='vertical', command=fvp.yview)
    fvp.configure(yscroll=fvpScroll.set)
    fvp['columns'] = ('Name', 'Props', 'Subs', 'Summary')
    fvp.column('#0', width=50)
    fvp.column('Name', width=70)
    fvp.column('Props', width=50) 
    fvp.column('Subs', width=50)
    fvp.column('Summary', width=250)
    fvp.heading('#0', text = 'No.')
    fvp.heading('Name', text = 'Name')
    fvp.heading('Props', text = 'Props')
    fvp.heading('Subs', text = 'Subs')
    fvp.heading('Summary', text = 'Summary')
    
    fvp.grid(sticky=(N,W,E,S))
    fvpScroll.grid(sticky=(N,S),row=0,column=1)

    showSelected = Button(root, text="Show Selected", command=lambda:showSelectedComp(log))
    extractEvents = Button(root, text="Extract Events", command=lambda:extractEvent(log))
    extractProps = Button(root, text = "Extract X-Props", command=lambda:extractXProp(log))

    showSelected.grid(sticky=W,row=2)
    extractEvents.grid(sticky=N, row=2)
    extractProps.grid(sticky=E, row=2)

    
    log = ScrolledText(root, width=80, height=10)
    log.grid(sticky=(W,S,E), pady=(200,0))
    
    clearButton = Button(root, text="CLEAR", command=lambda:log.delete(1.0,END))
    clearButton.grid(sticky=S)
    
    m=mainWindowMenu(root,fvp,log,cnx)
    root.configure(menu=m)


    return root

def init():

    if len(sys.argv) > 3:
        print("Insufficient Arguments, program terminating\n")
        sys.exit()
    elif len(sys.argv) < 2:
        print("Insufficient Arguments, program terminating\n")
        sys.exit()


    if len(sys.argv) == 2:
        username = sys.argv[1]
        for i in range(3):
            try:
                password = getpass.getpass("Enter your password (your input will not echo): ")
                cnx = mysql.connector.connect(user=username, password=password, host='dursley.socs.uoguelph.ca',database=username)
                return cnx
            except Error as e:
                print(e)
        
        print("You have exceeded 3 tries, program terminating..")
        sys.exit()

def createTables(cnx):
    try:
        query = "CREATE TABLE ORGANIZER( org_id INT AUTO_INCREMENT PRIMARY KEY, name VARCHAR( 60 ) NOT NULL, contact VARCHAR( 60 ) NOT NULL );"
        cursor = cnx.cursor()
        cursor.execute(query)
    except Error as e:
        print("Table ORGANIZER already exists, therefore it is not created")
    cursor.close()

    try:
        query = "CREATE TABLE EVENT( event_id INT AUTO_INCREMENT PRIMARY KEY, summary VARCHAR( 60 ) NOT NULL, start_time DATETIME NOT NULL, location VARCHAR ( 60 ), organizer INT, FOREIGN KEY( organizer ) REFERENCES ORGANIZER(org_id) ON DELETE CASCADE );"
    
        cursor = cnx.cursor()
        cursor.execute(query)
    except Error as e:
        print("Table EVENT already exists, therefore it is not created")
    cursor.close()

    try:
        query = "CREATE TABLE TODO( todo_id INT AUTO_INCREMENT PRIMARY KEY, summary VARCHAR ( 60 ) NOT NULL, priority SMALLINT, organizer INT, FOREIGN KEY(organizer ) REFERENCES ORGANIZER(org_id) ON DELETE CASCADE );"
        cursor = cnx.cursor()
        cursor.execute(query)
    except Error as e:
        print("Table TODO already exists, therefore it is not created")
    
    cursor.close()
cnx = init()
createTables(cnx)

mw = buildMainWindow(cnx)
mw.grid_columnconfigure(0, weight = 1)
mw.grid_rowconfigure(0, weight = 1)
try:
    os.environ['DATEMSK']
except KeyError:
    datemskPopup(mw)
mainloop()


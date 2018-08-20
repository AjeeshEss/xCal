/****calmodule.c****

Shared Object Library wrapper functions
Last Updated: April 7, 2016

Created: Wednesday March 16th, 2016
Author: Ajeesh Srijeyarajah (#0884990)
Contact: asrijeya@mail.uoguelph.ca
********************/
#include <Python.h>
#include "calutil.h"
#include <errno.h>

PyObject *Cal_readFile(PyObject * self, PyObject *args);
PyObject *Cal_writeFile(PyObject * self, PyObject *args);
PyObject *Cal_freeFile(PyObject * self, PyObject *args);
PyObject *Cal_getFVP(PyObject * self, PyObject *args);

static PyMethodDef CalMethods[] = {
    {"readFile", Cal_readFile, METH_VARARGS},
    {"writeFile", Cal_writeFile, METH_VARARGS},
    {"freeFile", Cal_freeFile, METH_VARARGS},
    {"getFVP", Cal_getFVP, METH_VARARGS},
    {NULL, NULL} };

static PyModuleDef calModuleDef = {
    PyModuleDef_HEAD_INIT,
    "Cal",
    NULL,
    -1,
    CalMethods};

PyMODINIT_FUNC PyInit_Cal(void){return PyModule_Create( &calModuleDef);}

PyObject *Cal_readFile(PyObject * self, PyObject *args){
    char * fileName;
    PyObject * result;
    PyObject * temp;

    if(!PyArg_ParseTuple(args,"sO", &fileName, &result)){
        return NULL;
    }

    FILE * ics = fopen(fileName, "r");
    if(ics == NULL){
        return Py_BuildValue("s", strerror(errno));
    }
    CalComp * comp;
    CalStatus status; 
    status = readCalFile(ics, &comp);

    if(status.code == OK){
        temp = Py_BuildValue("k", (unsigned long*)comp);
        PyList_Append(result, temp);
        for(int i = 0; i < comp->ncomps; i++){
            char * summary;
            CalProp * tempProp = comp->comp[i]->prop;
            while(tempProp != NULL){
                if(strcasecmp(tempProp->name,"SUMMARY")==0){
                    summary = malloc(sizeof(char)*(strlen(tempProp->value)+1));
                    strcpy(summary, tempProp->value);
                    break;
                }
                else{
                    summary = " ";
                }
                tempProp = tempProp->next;
            }
            temp = Py_BuildValue("siis", comp->comp[i]->name, comp->comp[i]->nprops, comp->comp[i]->ncomps, summary);
            PyList_Append(result, temp);
        }
        fclose(ics);       
        return Py_BuildValue("s", "OK");        
    }
    else{
        fclose(ics);
        char output[100];
        sprintf(output,"Error code %d at line %d to %d\n from file%s", status.code, status.linefrom, status.lineto, fileName);
        return Py_BuildValue("s", output);
    }
}
void removeIndex (int index, CalComp * comp){
    for(int i = index; i < comp->ncomps-1; i++){
        comp->comp[i] = comp->comp[i+1];
    }
    comp->ncomps--;
}
PyObject *Cal_writeFile(PyObject * self, PyObject *args){
    char * fromFileName;
    char * toFileName;
    int index;
    CalStatus status;
    CalComp * fvpComps;

    if(!PyArg_ParseTuple(args, "ssi", &fromFileName, &toFileName, &index)){
        return NULL;
    }
    if(index != -1){
        FILE * fvp = fopen(fromFileName, "w");
        if(fvp == NULL){
            return Py_BuildValue("s", strerror(errno));
        }
        FILE * dest = stdout;
        status = readCalFile(fvp, &fvpComps);
        if(status.code != OK){
            char output[100];
            sprintf(output, "Error code %d at line %d to %d from file %s\n", status.code, status.linefrom, status.lineto, fromFileName);
            return Py_BuildValue("s", output);
            
        } 
        status = writeCalComp(dest, fvpComps->comp[index]);
        if(status.code != OK){
            char output[100];
            sprintf(output, "Error code %d at line %d to %d\n", status.code, status.linefrom, status.lineto);
        }   
    }
    FILE * fvp = fopen(fromFileName, "w");
    if(fvp == NULL){
        return Py_BuildValue("s", strerror(errno));
    }
    FILE * dest = fopen(toFileName, "w");
    if(dest == NULL){
        return Py_BuildValue("s", strerror(errno));
    }


    status = readCalFile(fvp, &fvpComps);
    
    if(status.code != OK){
        char output[100];
        sprintf(output, "Error no. %d from line %d to %d in file %s\n", status.code, status.linefrom, status.lineto, fromFileName);
        fclose(fvp);
        return Py_BuildValue("s", output);
    }
    status = writeCalComp(dest, fvpComps);
    if(status.code != OK){
        char output[100];
        sprintf(output, "Error no. %d from line %d to %d in file%s\n", status.code, status.linefrom, status.lineto, toFileName);
        fclose(dest);
        return Py_BuildValue("s", output);


    }
    fclose(fvp);
    fclose(dest);                
    return Py_BuildValue("s", "OK");   
}
PyObject *Cal_freeFile(PyObject * self, PyObject *args){
    CalComp * pcal;

    if(!PyArg_ParseTuple(args, "k", (unsigned long*)&pcal)){
        return NULL;
    }
    freeCalComp(pcal);
    return Py_BuildValue("s", "OK");
}
PyObject *Cal_getFVP(PyObject * self, PyObject *args){
    PyObject * list;
    PyObject * temp;

    if(!PyArg_ParseTuple(args,"O", &list)){
        return NULL;
    }
    
    FILE * ics = fopen("fvp.ics", "r");

    CalComp * comp;
    readCalFile(ics, &comp);

    for(int i = 0; i < comp->ncomps; i++){
        if(strcmp(comp->comp[i]->name, "VEVENT")==0){
            CalProp * tempProp = comp->comp[i]->prop;
            char * summary;
            while(tempProp != NULL){
                if(strcasecmp(tempProp->name, "SUMMARY")==0){
                    summary = malloc(sizeof(char)*(strlen(tempProp->value)+1));
                    strcpy(summary, tempProp->value);
                    break;
                }
                else{
                    summary = "NULL";
                }
                tempProp = tempProp->next;
            }

            tempProp = comp->comp[i]->prop;
            char startDate[100];
           
            while(tempProp != NULL){
                if(strcasecmp(tempProp->name, "DTSTART")==0){
                    struct tm * tm = malloc(sizeof(struct tm));
                    tm->tm_isdst = -1;
                   
                    strptime(tempProp->value, "%Y%m%dT%H%M%S", tm);
                    time_t timenum = mktime(tm);

                    struct tm * tmTime = localtime(&timenum);
                    strftime(startDate, 100, "%Y-%m-%d %H:%M:%S", tmTime);
                    break;              
                }
                else{
                    
                }
                tempProp = tempProp->next;
            }
            
            tempProp = comp->comp[i]->prop;
            char * location;

            while(tempProp != NULL){
                if(strcasecmp(tempProp->name, "LOCATION")==0){
                    location = malloc(sizeof(char)*(strlen(tempProp->value)+1));
                    strcpy(location, tempProp->value);
                    break;
                }
                else{
                    location = "NULL";
                }
                tempProp = tempProp->next;
            }
            

            tempProp = comp->comp[i]->prop;
            char * name;
            char * contact;

            while(tempProp != NULL){
                if(strcasecmp(tempProp->name, "ORGANIZER")==0){
                    contact = malloc(sizeof(char)*(strlen(tempProp->value)+1));
                    strcpy(contact, tempProp->value);
                    name = malloc(sizeof(char)*(strlen(tempProp->param->value[0])+1));
                    strcpy(name, tempProp->param->value[0]);
                    break;
                }
                else{
                    contact = "NULL";
                    name = "NULL";
                }
                tempProp = tempProp->next;
            }
            temp = Py_BuildValue("sssss", summary, startDate, location, name, contact);
            PyList_Append(list,temp);           
        }
        else if(strcmp(comp->comp[i]->name, "VTODO")==0){
            
            CalProp * tempProp = comp->comp[i]->prop;
            char * summary;
            while(tempProp != NULL){
                if(strcasecmp(tempProp->name, "SUMMARY")==0){
                    summary = malloc(sizeof(char)*(strlen(tempProp->value)+1));
                    strcpy(summary, tempProp->value);
                    break;
                }
                else{
                    summary = "NULL";
                }
                tempProp = tempProp->next;
            }

            tempProp = comp->comp[i]->prop;
            int priority;

            while(tempProp != NULL){
                if(strcasecmp(tempProp->name, "PRIORITY")==0){
                    priority = atoi(tempProp->value);
                    break;
                }
                else{
                    priority = 1000;
                }
                tempProp = tempProp->next;
            }

            tempProp = comp->comp[i]->prop;
            char * name;
            char * contact;

            while(tempProp != NULL){
                if(strcasecmp(tempProp->name, "ORGANIZER")==0){
                    contact = malloc(sizeof(char)*(strlen(tempProp->value)+1));
                    strcpy(contact, tempProp->value);
                    name = malloc(sizeof(char)*(strlen(tempProp->param->value[0])+1));
                    strcpy(name, tempProp->param->value[0]);
                    break;
                }
                else{
                    contact = "NULL";
                    name = "NULL";
                }
                tempProp = tempProp->next;
            }
            temp = Py_BuildValue("siss", summary, priority, name, contact);
            PyList_Append(list, temp);

        }   
    }//end for
    fclose(ics);
    return Py_BuildValue("s", "OK");   
}

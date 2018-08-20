/****calutil.c****

iCalendar utility functions
Last Upated: Feb 13th, 2016

Created: Mon, Jan 8th, 2016
Author: Ajeesh Srijeyarajah (#0884990)
Contact: asrijeya@mail.uoguelph.ca


**********/

#define _GNU_SOURCE
#include "calutil.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#define BUFFSIZE 2048


/*checkVersion()

  Arguments: a pointer to a fully allocated Calendar struc
  Return values: returns OK if version is valid, BADVER if none or more than one are found

  Description: this function iterates through the comp's property list and checks each property to find a valid
  version.
*/
  CalError checkVersion(CalComp *pcomp){
    CalProp * temp = pcomp->prop; //temp variable pointing to
                                     // the first prop
    int found = 0; //0 = false, 1 = true
    int verCount = 0;//counts the number of times version found
    char * ver = "VERSION";
    char * num = "2.0";
    if(temp == NULL || pcomp->prop == NULL){
        return BADVER;
    }

    while(temp != NULL){
        if(strcmp(temp->name, ver)==0){
            verCount++;
            if(strcmp(temp->value, num)==0){
                found = 1;
            }
        }
        temp = temp->next;
    }

    if(verCount > 1){
        return BADVER;
    }
    if(found == 0){
        return BADVER;
    }

    return OK;
}
/*checkProdid()

  Arguments: pointer to a fully allocated calendar structure
  Return value: OK if valid prodid is found, NOPROD if more than one, or none are found

  Description: loops through the property list of the component check checks the PRODID for validity

*/

  CalError checkProdid(CalComp *pcomp){
    CalProp * temp = pcomp->prop;

    int idCount = 0;//counting howmany PRODID's are found
    int found = 0;//0 if not found, 1 if found

    char * id = "PRODID";
    if(temp == NULL){
        return NOPROD;
    }
    while(temp != NULL){
        if(strcmp(id, temp->name) == 0){
            idCount++;
            found = 1;
        }

        temp = temp->next;
    }

    if(idCount > 1){
        return NOPROD;
    }
    if(found == 0){
        return NOPROD;
    }

    return OK;
}
/*convertToUpper()

  Arguments: a pointer to an allocated string to convert
  Return value: the string is returned in upper case form

  Description: this function loops through the passed in string, and changes
  each character to uppercase individually
*/
  void convertToUpper(char * s){
    //while (*s) {
        //if ((*s >= 'a' ) && (*s <= 'z')) *s -= ('a'-'A');
        //s++;
    //}
    return;
}
/*checkAndFold
* Arguments: pointer to line to potentiall count, pointer to line count, file to write to
* returns: status (OK/IOERR)
* Description: if the line is over 75 chars, then it folds and prints, othewise it prints normally
*/
CalStatus checkAndFold (char * line, int * lineCount, FILE * ics){
    int length = strlen(line);
    char * topLine;
    char * foldedLine;
    char * copy;
    char * copy2;
    int ret = 0;
    CalStatus status;

    
    if(length > FOLD_LEN-1){
        topLine = malloc(sizeof(char*)*100);
        copy = malloc(sizeof(char*)*100);
        strcpy(copy,line);
        assert(copy!=NULL);
        strncpy(topLine,copy,FOLD_LEN-1);
        fprintf(ics,"%s \r\n",topLine);
        if(ret < 0){
            status.code = IOERR;
            status.linefrom = *lineCount;
            status.lineto = *lineCount;
            return status;
        }

        foldedLine = malloc(sizeof(char*)*100);
        foldedLine = malloc(sizeof(char*)*100);
        copy2 = malloc(sizeof(char*)*(strlen(line)+2));
        strcpy(copy2,line);
        assert(copy2!=NULL);
        strcpy(foldedLine,&copy2[FOLD_LEN]);
        ret = fprintf(ics," %s\r\n",foldedLine);
        if(ret < 0){
            status.code = IOERR;
            status.linefrom = *lineCount;
            status.lineto = *lineCount;
            return status;
        }
        (*lineCount) = (*lineCount) + 1;
    }
    else{
        ret = fprintf(ics,"%s\r\n",line);
        if(ret < 0){
            status.code = IOERR;
            status.linefrom = *lineCount;
            status.lineto = *lineCount;
            return status;
        }
    }
    return status;
}
/*WriteCalParam
* Arguments: head of param list, pointer to linecount, current line to add to
* Description: writeCalprop helper, takes in the line, and adds the parameters to it, and prints
*/
void writeCalParam(CalParam * head, int * lineCount, char ** outputLine){
    CalParam * current;

    current = head;
    while(current != NULL){
        
        sprintf((*outputLine)+ strlen(*outputLine), "%s=%s", current->name, current->value[0]);
        for(int i = 1; i < current->nvalues; i++){
            sprintf((*outputLine)+ strlen(*outputLine), ",%s", current->value[i]);
        }
        if(current->next == NULL){//if there are no more params
            sprintf((*outputLine) + strlen(*outputLine), ":");
        }
        else{
            sprintf((*outputLine)+ strlen(*outputLine), ";");
        }
        current = current->next;
    }

}
/*writeCalprop
* Arguments: head of prop list, pointer to line count, file to write to
* Returns: status containing OK or IOERR
* Description: Helper function for writecalcomp,processes properties to print
*/
CalStatus writeCalProp(CalProp * head, int * lineCount,FILE * ics){
    CalProp * current;
    char * outputLine;
    CalStatus status = {OK,0,0};

    current = head;
    while(current != NULL){
        if(current->nparams == 0){//if there is only 1 param
            outputLine = malloc(sizeof(char*)*1000);
            assert(outputLine != NULL);
            sprintf(outputLine, "%s:%s", current->name, current->value);
            status = checkAndFold(outputLine,lineCount,ics);
           /* if(status.code != OK){
                return status;
            }*/
            (*lineCount)++;
        }
        else{
            outputLine = malloc(sizeof(char*)*1000);
            assert(outputLine != NULL);
            sprintf(outputLine, "%s;", current->name);
            writeCalParam(current->param, lineCount, &outputLine);
            sprintf(outputLine + strlen(outputLine), "%s", current->value);
            status = checkAndFold(outputLine,lineCount,ics);
            
           /* if(status.code != OK){
                return status;
            }*/
            (*lineCount) = (*lineCount) + 1;

            free(outputLine);
        }
        current = current->next;
    }

    return status;
}

CalStatus writeCalComp(FILE*const ics, const CalComp *comp){
    CalStatus status;
    char * outputLine;
    static int lineCount = 0;
    int ret;
    //adds and prints BEGIN
    outputLine = malloc(sizeof(char*)*(strlen("BEGIN:\r\n") 
        + strlen(comp->name)+1));
    assert(outputLine != NULL);
    sprintf(outputLine, "BEGIN:%s\r\n", comp->name);
    ret = fprintf(ics,"%s",outputLine);
    if(ret < 0){
        status.code = IOERR;
        status.linefrom = lineCount;
        status.lineto = lineCount;
        return status;
    }
    lineCount++;
    writeCalProp(comp->prop, &lineCount,ics);
    if(comp->ncomps > 0){
        for(int i = 0; i < comp->ncomps; i++){
            status = writeCalComp(ics, comp->comp[i]);
        }
    }
    ret = fprintf(ics,"END:%s\r\n",comp->name);
    if(ret < 0){
        status.code = IOERR;
        status.linefrom = lineCount;
        status.lineto = lineCount;
        return status;
    }
    lineCount++;

    status.code = OK;
    status.linefrom = lineCount;
    status.lineto = lineCount;
    free(outputLine);

    return status;
}

CalStatus readCalFile(FILE *const ics, CalComp **const pcomp){

    CalStatus status;
    CalError error1, error2;
    status.code = OK;

    if(ics == NULL){
        status.code = NOCAL;
        status.lineto = 1;
        status.linefrom = 1;
    }

    readCalLine(NULL,NULL);
    //allocating the calendar
    (*pcomp) = malloc(sizeof(CalComp));
    assert((*pcomp)!=NULL);
    (*pcomp)->name = NULL;
    (*pcomp)->nprops = 0;
    (*pcomp)->prop = NULL;
    (*pcomp)->ncomps = 0;

    status = readCalComp(ics, pcomp);
    if(status.code != OK){
        free(*pcomp);
        return status;
    }

    //check version
    error1 = checkVersion(*pcomp);
    //check prodid
    error2 = checkProdid(*pcomp);

    if(error1 == OK && error2 != OK){
        status.code = error2;
    }
    else if(error1 != OK && error2 == OK){
        status.code = error1;
    }
    else if(error1 != OK && error2 != OK){
        status.code = BADVER;
    }

    if((*pcomp)->ncomps == 0){
        status.code = NOCAL;
    }

    return status;
    
}

CalStatus readCalComp(FILE *const ics, CalComp **const pcomp){

    static int nestCount = 0;//keeps track of the number of nested BEGINS
    CalProp * current;//temp ptr to help insert to prop linked list
    char * pbuff;//buffer to pass into ReadCalLine
    CalStatus status;
    status.code = OK;
    (*pcomp)->ncomps = 0;

    if(ics == NULL){
        status.code = NOCAL;
        nestCount = 0;
        return status;
    }

    
    if((*pcomp)->name == NULL && nestCount == 0){
        status = readCalLine(ics, &pbuff);
        if(strcasecmp(pbuff, "BEGIN:VCALENDAR")!= 0){
            status.code = NOCAL;
            return status;
        }
        else{
            (*pcomp)->name = malloc(sizeof(char*)*(strlen("VCALENDAR")+1));
            assert((*pcomp)->name!=NULL);
            sprintf((*pcomp)->name, "VCALENDAR");
            nestCount = 1;
        }
        
    }
    
    if(nestCount >= 1){//recursive case
        int read = 0;
        char endName[BUFFSIZE];
        convertToUpper((*pcomp)->name);
        sprintf(endName, "END:%s", (*pcomp)->name);//defining the END:V<name> string, for program to compar
                                                   //to know when the BEGIN block is done
        status = readCalLine(ics, &pbuff);
        if(strcasestr(pbuff, "END:") && !strcasestr(pbuff, "DTEND:")){
            status.code = NODATA;
            return status;
        }
        while(strcasecmp(pbuff,endName) != 0 && status.code == OK){//loops until it reads the desired END:V<name> string
            if(read == 1){
                status = readCalLine(ics, &pbuff);
            }
            else{
                read = 1;
            }
            if(pbuff == NULL){//EOF reached before end
                status.code = BEGEND;
                return status;
            }
            if(strcasestr(pbuff, "BEGIN:")){//if a nested BEGIN is found
                nestCount++;
                if(nestCount > 3){
                    status.code = SUBCOM;
                    return status;
                }
                char * token = strtok(pbuff,":");
                token = strtok(NULL,":");
                CalComp * newComp = malloc(sizeof(CalComp));//creating new comp struct to pass recursively
                assert(newComp != NULL);
                newComp->name = NULL;
                newComp->nprops = 0;
                newComp->prop = NULL;
                newComp->ncomps = 0;
                (*pcomp)->ncomps++;
                CalComp * ptr = realloc((*pcomp), sizeof(CalComp) 
                                + (*pcomp)->ncomps*sizeof(CalComp*));//reallocating flexible array
                //assert(ptr);
                if(ptr != NULL){
                    (*pcomp) = ptr;
                }
                newComp->name = malloc(sizeof(char*)*(strlen(token)+1));
                assert(newComp->name!=NULL);
                strcpy(newComp->name, token);
                status = readCalComp(ics, &newComp);//calls function recursively
                (*pcomp)->comp[(*pcomp)->ncomps - 1] = newComp;//adding new comp to flexible array

            }
            else if(strcasestr(pbuff, "END:") 
                    && !strcasestr(pbuff, "DTEND:")){//if an END is read

                convertToUpper(pbuff);
            nestCount--;
                if((*pcomp)->nprops == 0 && (*pcomp)->ncomps == 0){//if no properties are found, returns NODATA
                    status.code = NODATA;
                    return status;
                }
                if(strcasecmp(pbuff, endName) != 0){//if it is the wrong END block, returns BEGEND
                    status.code = BEGEND;
                    return status;
                }  
            }
            else{//reading properties as normal
                CalProp * property;
                property = malloc(sizeof(CalProp));
                assert(property!=NULL);
                status.code = parseCalProp(pbuff, property);//passing property struct to be filled in

                
                if((*pcomp)->prop == NULL){//adding to head of prop list
                    property->next = NULL;
                    (*pcomp)->prop = property;
                }
                else{//ending to back of prop list
                    current = (*pcomp)->prop;
                    while(current->next != NULL){
                        current = current->next;
                    }
                    current->next = property;


                }
                (*pcomp)->nprops++;
            }
        }
    }
    
    return status;

}

CalStatus readCalLine(FILE *const ics, char **const pbuff){

    char tempBuffer[BUFFSIZE] = "";//temporary
    char tempBuffer2[BUFFSIZE] = "";//buffers
    char noSpaceBuff[BUFFSIZE] = "";//buffer containing a string with leading space removed(for unfolding)
    char beginChar;//char to hold the first character of the line, to check for folding
    static int lineNumber = 0;//holds the number of lines read
    int length;
    int boolFolded = 0;
    CalStatus status;
    
    if(ics == NULL){//special call, resets all variables and such
        status.code = OK;
        status.linefrom = 0;
        status.lineto = 0;
        beginChar = ' ';
        lineNumber = 0;
        length = 0;
        //reset other things here

        return status;
    }

    if(fgets(tempBuffer,BUFFSIZE,ics)!= NULL){//reading a line
        tempBuffer[strcspn(tempBuffer, "\r\n")] = 0;
        lineNumber++;
        (*pbuff) = NULL;
        if(strcmp(tempBuffer,"") == 0 ||
         strcmp(tempBuffer," ") == 0 ||
         strcmp(tempBuffer,"\t") == 0 ||
           strcmp(tempBuffer,"\r\n") == 0){//if the line is empty (CRLF, blank space, tab etc), ignore it and read the next

            fgets(tempBuffer,BUFFSIZE,ics);
        tempBuffer[strcspn(tempBuffer, "\r\n")] = 0;
        lineNumber++;
    }

    beginChar = fgetc(ics);
        if(beginChar == ' ' || beginChar == '\t'){//checks the next line for a space, if there is, then it is folded
            ungetc(beginChar, ics);//puts char back
            boolFolded = 1;
            fgets(tempBuffer2, BUFFSIZE,ics);//gets the next line to combine
            tempBuffer2[strcspn(tempBuffer2, "\r\n")] = 0;
            for(int i = 1; i < strlen(tempBuffer2); i++){
                noSpaceBuff[i-1] = tempBuffer2[i];
            }
            
            tempBuffer[strcspn(tempBuffer, "\r\n")] = 0;//removing the CRLF from the first line for proper formatting
            strcat(tempBuffer,noSpaceBuff);//combining the 2 lines together
            length = strlen(tempBuffer) + 1;
            (*pbuff) = malloc(sizeof(char*)*length);
            assert((*pbuff)!=NULL);
            strcpy((*pbuff), tempBuffer);
        }
        else{//if there is no folding, read line like normal
            ungetc(beginChar,ics);
            length = strlen(tempBuffer) + 1;
            (*pbuff) = malloc(sizeof(char*)*length);
            assert((*pbuff)!=NULL);
            strcpy((*pbuff), tempBuffer);
        }
    }
    else if(fgets(tempBuffer,BUFFSIZE,ics)==NULL){
        (*pbuff) = NULL;
    }

    //if(!strstr((*pbuff), "\r\n")){//if the buffer does not have CRNL, it returns NOCRNL
    //    status.code = NOCRNL;
    //}
    //else{
    status.code = OK;
    //}
    status.linefrom = lineNumber;

    if(boolFolded == 1){
        lineNumber++;
    }
    status.lineto = lineNumber;


    return status;
}

CalError parseCalProp(char *const buff, CalProp *const prop){

    //initialize pointer fields to null
    //initialize params to 0
    prop->name = NULL;
    prop->value = NULL;
    prop->nparams = 0;
    prop->param = NULL;
    prop->next = NULL;

    char * tempRest;



    if(!strstr(buff,":")){//if there is no colon, then it violates the syntax
        return SYNTAX;
    }
    if(!strstr(buff, ";")){//if the buffer has no ;, then there are no parameters, and can be parsed
                           // as NAME:VALUE
        if(buff[0] == ':'){
            return SYNTAX;
        }
        char * token = strtok(buff,":");
        prop->name = malloc(sizeof(char*)*(strlen(token)+1));
        assert(prop->name!=NULL);
        strcpy(prop->name, token);
        convertToUpper(prop->name);
        token = strtok(NULL,"");
        if(token != NULL){
            prop->value = malloc(sizeof(char*)*(strlen(token)+1));
            assert(prop->value!=NULL);
            strcpy(prop->value, token);
        }
        else{
            prop->value = malloc(sizeof(char*)*(strlen("")+1));
            assert(prop->value!=NULL);
            strcpy(prop->value, "");
        }

    }
    else if(strstr(buff, ";")){
        if(strchr(buff,':') < strchr(buff,';')){//if the colon appears before the semi, then there are no
                                                //parameters
            char * token = strtok(buff,":");
            prop->name = malloc(sizeof(char*)*(strlen(token)+1));
            assert(prop->name!=NULL);
            strcpy(prop->name, token);
            token = strtok(NULL," ");
            prop->value = malloc(sizeof(char*)*(strlen(token)+1));
            assert(prop->value!=NULL);
            strcpy(prop->value, token);
        }
        else{//There are parameters to be parsed
            CalParam * current;
            char * rest;
            char * token = strtok_r(buff,";", &rest);
            prop->name = malloc(sizeof(char*)*(strlen(token)+1));
            assert(prop->name != NULL);
            strcpy(prop->name, token);
            token = strtok_r(NULL, "", &rest);
            while(token != NULL){
                CalParam * param;
                param = malloc(sizeof(CalParam));
                assert(param!=NULL);
                param->nvalues = 0;
                param->next = NULL;
                token = strtok_r(token, "=", &rest);
                param->name = malloc(sizeof(char*)*(strlen(token)+1));
                assert(param->name!=NULL);
                //convertToUpper(token);
                strcpy(param->name, token);

                if(strstr(rest, ";")){
                    token = strtok_r(NULL, ";", &rest);
                    if(strstr(token, ",")){
                        char * temp = strdup(token);
                        assert(temp!=NULL);

                        temp = strtok_r(temp, ",", &tempRest);
                        while(temp!= NULL){
                            param->nvalues++;
                            int length = param->nvalues;
                            param = realloc(param,sizeof(CalParam) + (sizeof(char*)*length));
                            assert(param != NULL);
                            param->value[length-1] = malloc(sizeof(char*)*(strlen(token)+1));
                            assert(param->value[length-1]!=NULL);
                            strcpy(param->value[length-1], token);

                            temp = strtok_r(NULL, ",", &tempRest);
                        }
                        if(prop->param == NULL){//adding to head of prop list
                            param->next = NULL;
                            prop->param = param;
                        }
                            else{//inserting to back of prop list
                                current = prop->param;
                                while(current->next != NULL){
                                    current = current->next;
                                }  
                                current->next = param;
                            }
                            prop->nparams++;
                        }
                        else{
                            param->nvalues++;
                            int length = param->nvalues;
                            param = realloc(param,sizeof(CalParam) + (sizeof(char*)*length));
                            assert(param!=NULL);
                            param->value[length-1] = malloc(sizeof(char*)*(strlen(token)+1));
                            assert(param->value[length-1]!=NULL);
                            //convertToUpper(token);
                            strcpy(param->value[length-1], token);
                        if(prop->param == NULL){//adding to head of prop list
                            param->next = NULL;
                            prop->param = param;
                        }
                        else{//inserting to back of prop list
                            current = prop->param;
                            while(current->next != NULL){
                                current = current->next;
                            }
                            current->next = param;
                        }
                        prop->nparams++;
                    }
                   // if(strstr(rest, ";")){
                    token = strtok_r(NULL, "", &rest);
                    //}
                }

                else{
                    token = strtok_r(NULL, ":", &rest);
                    char * colon = strrchr(token,':');
                    if(colon != NULL){
                        token = strtok_r(colon,":", &rest);
                    }
                    if(strstr(token, ",")){
                        char * temp = strdup(token);
                        assert(temp!=NULL);

                        temp = strtok_r(temp, ",", &tempRest);
                        while(temp!= NULL){
                            param->nvalues++;
                            int length = param->nvalues;
                            param = realloc(param,sizeof(CalParam) + (sizeof(char*)*length));
                            assert(param!=NULL);
                            param->value[length-1] = malloc(sizeof(char*)*(strlen(temp)+1));
                            assert(param->value[length-1]!=NULL);
                           //convertToUpper(temp);
                            strcpy(param->value[length-1], temp);
                            temp = strtok_r(NULL, ",", &tempRest);

                        }
                        if(prop->param == NULL){//adding to head of prop list
                            param->next = NULL;
                            prop->param = param;
                        }
                        else{//inserting to back of prop list
                            current = prop->param;
                            while(current->next != NULL){
                                current = current->next;
                            }
                            current->next = param;
                        }
                        prop->nparams++;
                    }
                    else{
                        param->nvalues++;
                        int length = param->nvalues;
                        param = realloc(param,sizeof(CalParam) + (sizeof(char*)*length));
                        assert(param!=NULL);
                        param->value[length-1] = malloc(sizeof(char*)*(strlen(token)+1));
                        assert(param->value[length-1]!=NULL);
                        convertToUpper(token);
                        strcpy(param->value[length-1], token);
                        if(prop->param == NULL){//adding to head of prop list
                            param->next = NULL;
                            prop->param = param;
                        }
                        else{//inserting to back of prop list
                            current = prop->param;
                            while(current->next != NULL){
                                current = current->next;
                            }
                            current->next = param;
                        }
                        prop->nparams++;
                    }
                    token = strtok_r(NULL, "", &rest);
                    prop->value = malloc(sizeof(char*)*(strlen(token)+1));
                    assert(prop->value!=NULL);
                    strcpy(prop->value, token);
                    
                    break;
                }
            }
        }
    }


    return OK;
}

void freeCalComp(CalComp *const comp){

    CalProp * prop;
    CalProp * temp;

    prop = comp->prop;
    while(prop != NULL){
        temp = prop;
        prop = prop->next;
        free(temp->name);
        free(temp->value);
        free(temp->param);
        free(temp);
    }

    for(int i = 0; i < comp->ncomps; i++){
        if(comp->comp[i] != NULL){
            freeCalComp(comp->comp[i]);
        }
    }
}

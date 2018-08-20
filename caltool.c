/****caltool.c****

iCalendar command line functionality
Last Upated: Feb 22nd, 2016

Created: Mon,Feb 15, 2016
Author: Ajeesh Srijeyarajah (#0884990)
Contact: asrijeya@mail.uoguelph.ca


**********/
#define _GNU_SOURCE
#include "caltool.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>

#define TIMEBUFFSIZE 100
/*calEvent struct
* This is a helper struct for extract, it stores
* the time and summary for each event, to make it easier
* to sort
*/
typedef struct calEvent{
	time_t theTime;
	char * summary;
} calEvent;

/*printExitStatus
* Arguments: Calendar status, file to write to
* Description: reads the status, and prints to txtfile any error and line number
*/
void printExitStatus(CalStatus status, FILE * txtFile){

	
	if(status.code == 1){
		fprintf(txtFile,"AFTEND: Found more text after end of cal\n");
	}
	else if(status.code == 2){
		fprintf(txtFile,"BADVER: Incorrect or missing version\n");
	}
	else if(status.code == 3){
		fprintf(txtFile,"BEGEND: No end to comp beginning\n");
	}
	else if(status.code == 4){
		fprintf(txtFile,"IOERR: Error with file I/O\n");
	}
	else if(status.code == 5){
		fprintf(txtFile,"NOCAL: No components in cal\n");	
	}
	else if(status.code == 6){
		fprintf(txtFile,"NOCRNL: No CRNL in line \n");
	}
	else if(status.code == 7){
		fprintf(txtFile,"NODATA: No properties or components\n");
	}
	else if(status.code == 8){
		fprintf(txtFile,"NOPROD: Prodid not found\n");
	}
	else if(status.code == 9){
		fprintf(txtFile,"SUBCOM: Exceeed number of components\n");
	}
	else if(status.code == 10){
		fprintf(txtFile,"SYNTAX: Property Syntax Error\n");
	}

	fprintf(txtFile,"At line %d to %d\n", status.linefrom, status.lineto);
}
/*checkIOERR
* Arguments: integer containing fprintf return val
* Return: returns IOERR if one was found, or OK if not
* Description: this function looks at the printf return val
* if it is less than zero, then printf failed (IOERR)
*/
CalError checkIOERR(int ret){
	if (ret < 0){
		return IOERR;
	}
	else{
		return OK;
	}
}
/* countComponents
*  Arguments: pointer to a calendar, pointer to # of line sprinted, txtfile to write to
*  Returns: returns OK if successful, IOERR if not
*  Description: Reads through every component in the calendar, and prints info such as
*  number of components, events, todo, others.
*/
CalError countComponents(const CalComp * comp, int * linesPrinted, FILE * txtFile){
	int ncomponents = 0; 
	int nevents = 0; 
	int ntodo = 0;
	int nothers = 0;
	int ret;
	CalError code;
	ncomponents = comp->ncomps;

	for(int i = 0; i < ncomponents; i++){
		if(strcmp(comp->comp[i]->name, "VEVENT") == 0){
			nevents++;
		}
		else if(strcmp(comp->comp[i]->name, "VTODO") == 0){
			ntodo++;
		}
		else{
			nothers++;
		}
	}
	if(ncomponents == 1){
		ret = fprintf(txtFile,"%d component: ", ncomponents);
	}
	else{
		ret = fprintf(txtFile,"%d components: ", ncomponents);
	}
	if(checkIOERR(ret) != OK){//checking for IOERR, used everytime fprintf is called throughout
		return IOERR;
	}
	if(nevents == 1){
		ret = fprintf(txtFile,"%d event, ", nevents);
	}
	else{
		ret = fprintf(txtFile,"%d events, ", nevents);
	}
	if(checkIOERR(ret) != OK){
		return IOERR;
	}

	if(ntodo == 1){
		ret = fprintf(txtFile,"%d todo, ", ntodo);
	}
	else{
		ret = fprintf(txtFile,"%d todos, ", ntodo);
	}
	if(checkIOERR(ret) != OK){
		return IOERR;
	}

	if(nothers == 1){
		ret = fprintf(txtFile,"%d other\n", nothers);
	}
	else{
		ret = fprintf(txtFile,"%d others\n", nothers);
	}
	if(checkIOERR(ret) != OK){
		return IOERR;
	}

	*linesPrinted = *linesPrinted + 1;	
	code = OK;

	return code;


}
/*countSubComponents
* Arguments: pointer to cal struct, pointer to lines printed, txtfile to write to
* Returns: OK if successful, IOERR if no
* Description: counts the number of subcomponents in the calendar
*/
CalError countSubComponents(const CalComp * comp, int * linesPrinted,FILE * txtFile){
	int nsubcomponents = 0;
	int ret;

	CalError code;

	for(int i = 0; i < comp->ncomps; i++){
		nsubcomponents = nsubcomponents + comp->comp[i]->ncomps;
	}
	if(nsubcomponents == 1){
		ret = fprintf(txtFile,"%d subcomponent\n", nsubcomponents);
	}
	else{
		ret = fprintf(txtFile,"%d subcomponents\n", nsubcomponents);
	}
	code = checkIOERR(ret);
	if(code == OK){
		*linesPrinted = *linesPrinted + 1;
	}

	return code;
}
/*countProperties
* Arguments: pointer to calendar struct
* Return: integer representing the number of properties in the calendar
* Description: counts the number of props in the calendar
*/
int countProperties(const CalComp * comp){
	static int nproperties = 0;//holds the number of properties found
	CalProp * current;

	current = comp->prop;

	while(current != NULL){
		nproperties++;
		current = current->next;
	}

	for(int i = 0; i < comp->ncomps; i++){
		nproperties = countProperties(comp->comp[i]);
	}

	return nproperties;

}
/*compareNames
* THIS FUNCTION IS FOR QSORT, TO SORT ORGANIZER NAMES
* function based off of: http://www.anyexample.com/programming/c/qsort__sorting_array_of_strings__integers_and_structs.xml
*/
int compareNames(const void * firstName, const void * secondName){
	const char * temp1 = *(const char**)firstName;
	const char * temp2 = *(const char**)secondName;

	return strcmp(temp1, temp2);
}
int nameExists(char ** array, char * name, int arraySize){
	for(int i = 0; i < arraySize; i++){
		if(array[i]!= NULL){
			if(strcmp(array[i],name) == 0){
				return 1;
			}
		}
	}
	return 0;
}
/*getLatestDate
* Arguments: array of times, length of array
* Returns: Latest time
* Description: finds the most recent time (for range of dates)
*/
time_t getLatestDate(time_t * timeArray,int length){
	time_t maximum = 0;

	for(int i = 0;i < length; i++){
		if(timeArray[i]>maximum){
			maximum = timeArray[i];
		}
	}

	return maximum;

}
/*getEarliestDate
* Arguments: array of times, length of array
* Returns: earliest time
* Description: finds the least recent time (for range of dates)
*/
time_t getEarliestDate(time_t * timeArray,int length){
	time_t minimum = timeArray[0];

	for(int i = 0; i < length; i++){
		if(timeArray[i] < minimum){
			minimum = timeArray[i];
		}
	}

	return minimum;
}
/*
* Arguments: cal struct, file to write to, pointer to array of times and length, pointer that,
* represents whether times were found
* Description: loops through every property and stores all the dates in an array to be sorted later
*/
void getDateRange(const CalComp * comp, FILE *const txtFile, time_t ** timeArray,int*len,int * foundTimes){
	CalProp * current;
	time_t * internalArray;//array to hold every time found in the file
	int length = 0;//length of internal array


	internalArray = malloc(sizeof(time_t)*length);
	for(int o = 0; o < comp->ncomps; o++){
                if(strcmp(comp->comp[o]->name, "VTIMEZONE")!= 0){
		current = comp->comp[o]->prop;
		while(current != NULL){
			if(strcmp(current->name,"COMPLETED" )==0){
				struct tm * tm = malloc(sizeof(struct tm));
				tm->tm_isdst = -1;
				assert(tm!=NULL);

				strptime(current->value, "%Y%m%dT%H%M%S", tm);
				time_t timeNum = mktime(tm);
				length++;

				internalArray = realloc(internalArray,sizeof(time_t)*length);
				internalArray[length-1] = timeNum;
				*foundTimes = 1;





				free(tm);
			}
			else if(strcmp(current->name,"DTEND" )==0){
				struct tm * tm = malloc(sizeof(struct tm));
				tm->tm_isdst = -1;
				assert(tm!=NULL);

				strptime(current->value, "%Y%m%dT%H%M%S", tm);
				time_t timeNum = mktime(tm);
				length++;

				internalArray = realloc(internalArray,sizeof(time_t)*length);
				internalArray[length-1] = timeNum;
				*foundTimes = 1;



				free(tm);
			}
			else if(strcmp(current->name,"DUE" )==0){
				struct tm * tm = malloc(sizeof(struct tm));
				tm->tm_isdst = -1;
				assert(tm!=NULL);

				strptime(current->value, "%Y%m%dT%H%M%S", tm);
				time_t timeNum = mktime(tm);
				length++;

				internalArray = realloc(internalArray,sizeof(time_t)*length);
				internalArray[length-1] = timeNum;
				*foundTimes = 1;





				free(tm);
			}
			else if(strcmp(current->name,"DTSTART" )==0){
				struct tm * tm = malloc(sizeof(struct tm));
				tm->tm_isdst = -1;
				assert(tm!=NULL);

				strptime(current->value, "%Y%m%dT%H%M%S", tm);
				time_t timeNum = mktime(tm);
				length++;

				internalArray = realloc(internalArray,sizeof(time_t)*length);
				internalArray[length-1] = timeNum;
				*foundTimes = 1;



				free(tm);
			}
			else if((strcmp(current->name,"CREATED" )==0)){
				struct tm * tm = malloc(sizeof(struct tm));
				tm->tm_isdst = -1;
				assert(tm!=NULL);

				strptime(current->value, "%Y%m%dT%H%M%S", tm);
				time_t timeNum = mktime(tm);
				length++;

				internalArray = realloc(internalArray,sizeof(time_t)*length);
				internalArray[length-1] = timeNum;
				*foundTimes = 1;




				free(tm);

			}
			else if(strcmp(current->name,"DTSTAMP" )==0){
				struct tm * tm = malloc(sizeof(struct tm));
				tm->tm_isdst = -1;
				assert(tm!=NULL);

				strptime(current->value, "%Y%m%dT%H%M%S", tm);
				time_t timeNum = mktime(tm);
				length++;

				internalArray = realloc(internalArray,sizeof(time_t)*length);
				internalArray[length-1] = timeNum;
				*foundTimes = 1;





				free(tm);
			}
			else if(strcmp(current->name,"LAST-MODIFIED" )==0){
				struct tm * tm = malloc(sizeof(struct tm));
				tm->tm_isdst = -1;
				assert(tm!=NULL);

				strptime(current->value, "%Y%m%dT%H%M%S", tm);
				time_t timeNum = mktime(tm);
				length++;

				internalArray = realloc(internalArray,sizeof(time_t)*length);
				internalArray[length-1] = timeNum;
				*foundTimes = 1;



				free(tm);

			}
			current = current->next;
		}

		for(int i = 0; i < comp->comp[o]->ncomps; i++){
			current = comp->comp[o]->comp[i]->prop;
			while(current != NULL){
				if(strcmp(current->name,"COMPLETED" )==0){
					struct tm * tm = malloc(sizeof(struct tm));
					tm->tm_isdst = -1;
					assert(tm!=NULL);

					strptime(current->value, "%Y%m%dT%H%M%S", tm);
					time_t timeNum = mktime(tm);
					length++;

					internalArray = realloc(internalArray,sizeof(time_t)*length);
					internalArray[length-1] = timeNum;
					*foundTimes = 1;





					free(tm);
				}
				else if(strcmp(current->name,"DTEND" )==0){
					struct tm * tm = malloc(sizeof(struct tm));
					tm->tm_isdst = -1;
					assert(tm!=NULL);

					strptime(current->value, "%Y%m%dT%H%M%S", tm);
					time_t timeNum = mktime(tm);
					length++;

					internalArray = realloc(internalArray,sizeof(time_t)*length);
					internalArray[length-1] = timeNum;
					*foundTimes = 1;



					free(tm);
				}
				else if(strcmp(current->name,"DUE" )==0){
					struct tm * tm = malloc(sizeof(struct tm));
					tm->tm_isdst = -1;
					assert(tm!=NULL);

					strptime(current->value, "%Y%m%dT%H%M%S", tm);
					time_t timeNum = mktime(tm);
					length++;

					internalArray = realloc(internalArray,sizeof(time_t)*length);
					internalArray[length-1] = timeNum;
					*foundTimes = 1;




					free(tm);
				}
				else if(strcmp(current->name,"DTSTART" )==0){
					struct tm * tm = malloc(sizeof(struct tm));
					tm->tm_isdst = -1;
					assert(tm!=NULL);

					strptime(current->value, "%Y%m%dT%H%M%S", tm);
					time_t timeNum = mktime(tm);
					length++;

					internalArray = realloc(internalArray,sizeof(time_t)*length);
					internalArray[length-1] = timeNum;
					*foundTimes = 1;





					free(tm);
				}
				else if((strcmp(current->name,"CREATED" )==0)){
					struct tm * tm = malloc(sizeof(struct tm));
					tm->tm_isdst = -1;
					assert(tm!=NULL);

					strptime(current->value, "%Y%m%dT%H%M%S", tm);
					time_t timeNum = mktime(tm);
					length++;

					internalArray = realloc(internalArray,sizeof(time_t)*length);
					internalArray[length-1] = timeNum;
					*foundTimes = 1;




					free(tm);

				}
				else if(strcmp(current->name,"DTSTAMP" )==0){
					struct tm * tm = malloc(sizeof(struct tm));
					tm->tm_isdst = -1;
					assert(tm!=NULL);

					strptime(current->value, "%Y%m%dT%H%M%S", tm);
					time_t timeNum = mktime(tm);
					length++;

					internalArray = realloc(internalArray,sizeof(time_t)*length);
					internalArray[length-1] = timeNum;
					*foundTimes = 1;





					free(tm);
				}
				else if(strcmp(current->name,"LAST-MODIFIED" )==0){
					struct tm * tm = malloc(sizeof(struct tm));
					assert(tm!=NULL);

					strptime(current->value, "%Y%m%dT%H%M%S", tm);
					time_t timeNum = mktime(tm);
					length++;

					internalArray = realloc(internalArray,sizeof(time_t)*length);
					internalArray[length-1] = timeNum;
					*foundTimes = 1;




					free(tm);

				}
				current = current->next;
			}
		}
	}
        }
	*len = length;
	*timeArray = internalArray;
}

CalStatus calInfo(const CalComp *comp, int lines, FILE *const txtFile){

	CalStatus status;
	int linesPrinted = 0;
	int length = 0;
	int found = 0;
	int foundTimes = 0;
	int ret;
	time_t * timeArray;
	time_t earlyTime;
	time_t lateTime;


			//number of lines
	if(lines == 1){
		ret = fprintf(txtFile,"%d line\n", lines);
	}
	else{
		ret = fprintf(txtFile,"%d lines\n", lines);
	}
	status.code = checkIOERR(ret);
	if(status.code != OK){
		status.linefrom = linesPrinted;
		status.lineto = linesPrinted;
		return status;
	}
	else{
		linesPrinted++;
	}

			//number of components
	status.code = countComponents(comp,&linesPrinted,txtFile);
	if(status.code != OK){
		status.linefrom = linesPrinted;
		status.lineto = linesPrinted;
		return status;
	}

			//number of subcomponents
	status.code = countSubComponents(comp, &linesPrinted, txtFile);
	if(status.code != OK){
		status.linefrom = linesPrinted;
		status.lineto = linesPrinted;
		return status;
	}

			//number of properties
	int nproperties = countProperties(comp);
	if(nproperties == 1){
		ret = fprintf(txtFile,"%d property\n", nproperties);
		status.code = (checkIOERR(ret));
	}
	else{
		ret = fprintf(txtFile,"%d properties\n", nproperties);
		status.code = (checkIOERR(ret));
	}

	if(status.code != OK){
		status.linefrom = linesPrinted;
		status.lineto = linesPrinted;
		return status;
	}
	else{
		linesPrinted++;
	}
			//Range of dates
	getDateRange(comp,txtFile,&timeArray,&length,&foundTimes);
	if(foundTimes == 1){
		earlyTime = getEarliestDate(timeArray, length);
		struct tm * earlyTm = localtime(&earlyTime);

		char earlyStr[TIMEBUFFSIZE];
		strftime(earlyStr,TIMEBUFFSIZE,"%Y-%b-%d", earlyTm);
		ret = fprintf(txtFile,"From %s ",earlyStr);

		status.code = checkIOERR(ret);
		if(status.code != OK){
			status.linefrom = linesPrinted;
			status.lineto = linesPrinted;
			return status;
		}

		lateTime = getLatestDate(timeArray, length);
		struct tm * lateTm = localtime(&lateTime);
		char lateStr[TIMEBUFFSIZE];

		strftime(lateStr,TIMEBUFFSIZE,"%Y-%b-%d", lateTm);
		ret = fprintf(txtFile,"to %s\n",lateStr);

		status.code = checkIOERR(ret);
		if(status.code != OK){
			status.linefrom = linesPrinted;
			status.lineto = linesPrinted;
			return status;
		}
		linesPrinted++;
	}
	else{
		ret = fprintf(txtFile,"No dates\n");
		status.code = checkIOERR(ret);
		if(status.code != OK){
			status.linefrom = linesPrinted;
			status.lineto = linesPrinted;
			return status;
		}
		linesPrinted++;
	}



	status.code = checkIOERR(ret);
	if(status.code != OK){
		status.linefrom = linesPrinted;
		status.lineto = linesPrinted;
		return status;
	}
	else{
		linesPrinted++;
	}
	//organizers
	int numNames = 0;//length of names
	char ** names = malloc(sizeof(char*)*numNames);//list of organizers found (no duplicates)
	assert(names!=NULL);
	CalProp * current;
	int arrayLength = 0;//length of allNames
	char ** allNames = NULL;//list of all organizers found in total
	int printorg = 0;//if 0, org has not been printed

	for(int i = 0; i < comp->ncomps; i++){
		current = comp->comp[i]->prop;
		while(current != NULL){
			if(strcmp(current->name, "ORGANIZER") == 0){
				found = 1;
				CalParam * temp = current->param;
				while(temp != NULL){

					if(strcmp(temp->name, "CN") == 0){
						//if no duplicate is found, add to master list and current list
						if(nameExists(allNames, temp->value[0], arrayLength) != 1){
							arrayLength++;
							allNames = realloc(allNames,sizeof(char*)*arrayLength);
							assert(allNames != NULL);
							allNames[arrayLength-1] = temp->value[0];
							numNames++;
							names = realloc(names,sizeof(char*)*numNames);
							assert(names != NULL);
							names[numNames-1] = temp->value[0];
						}
					}
					temp = temp->next;
				}
			}
			current = current->next;
		}
	}
	if(found == 0){
		ret = fprintf(txtFile,"No organizers\n");
		status.code = checkIOERR(ret);
		if(status.code != OK){
			status.linefrom = linesPrinted;
			status.lineto = linesPrinted;
			return status;
		}
		linesPrinted++;
	}
	else{//if organizers have been found
		qsort(names, numNames, sizeof(char*), compareNames);

		for(int j = 0; j < numNames; j++){
			if(strcmp(names[j]," ")!= 0){//removing any blanks
				if(printorg == 0){//variable so that Organizers: is only printed once
					fprintf(txtFile,"Organizers:\n");
					printorg = 1;
				}
				int ret = fprintf(txtFile,"%s", names[j]);
				status.code = checkIOERR(ret);
				if(status.code == OK){
					linesPrinted++;
					fprintf(txtFile,"\n");
				}
				else{
					status.linefrom = linesPrinted;
					status.lineto = linesPrinted;
					return status;
				}

			}
		}

	}


	status.code = OK;
	status.lineto = linesPrinted;
	status.linefrom = linesPrinted;


	return status;

}
/*checkDuplicateXprop
* Arguments: array of X-property pointer, length of array, name of prop to checl
* Returns: 1 if a duplicate is found, 0 otherwise
* Description: loops through array of properties and finds a duplicate
*/
int checkDuplicateXProp(char**propArray,int propLength, char*propName){
	int found = 0;//integer representing if duplicate is found

	for(int i = 0; i < propLength; i++){
		if(strcmp(propArray[i],propName)==0){
			found = 1;
		}
	}

	return found;
}
/*compareTimes
* For QSORT, for list of events in calExtract, to sort by recent to late
*/
int compareTimes(const void * a, const void * b){
	struct calEvent * time1 = (struct calEvent*)a;
	struct calEvent * time2 = (struct calEvent*)b;
	if(time1->theTime == time2->theTime){
		return 0;
	}
	else if(time1->theTime > time2->theTime){
		return 1;
	}
	else{
		return -1;
	}
}
CalStatus calExtract(const CalComp *comp, CalOpt kind, FILE *const txtfile){
	CalStatus status;

	if(kind == OEVENT){//extracting events
		struct calEvent * eventArray;//array to hold any events found
		struct calEvent event;
		int numEvents = 0;//length of eventArray

		eventArray = malloc(sizeof(struct calEvent*)*numEvents);
		assert(eventArray);
		for(int i = 0; i < comp->ncomps; i++){
			int summaryFound = 0;//int that represent if summary prop was found r not
			if(strcmp(comp->comp[i]->name,"VEVENT")==0){
				CalProp * current = comp->comp[i]->prop;
				while(current!=NULL){
					if(strcmp(current->name,"DTSTART")==0){
						struct tm * tm = malloc(sizeof(struct tm));
						tm->tm_isdst = -1;
						assert(tm!=NULL);

						strptime(current->value, "%Y%m%dT%H%M%S", tm);
						time_t timeNum = mktime(tm);

						event.theTime = timeNum;
					}
					if(strcmp(current->name,"SUMMARY")==0){
						event.summary = malloc(sizeof(char)*(strlen(current->value)+1));
						assert(event.summary);
						strcpy(event.summary,current->value);
						summaryFound = 1;
					}
					current = current->next;
				}
				if(summaryFound == 0){//if no summary was found, default to (na)
					event.summary = malloc(sizeof(char)*(strlen("(na)")+1));
					assert(event.summary);
					strcpy(event.summary,"(na)");
				}
				numEvents++;
				eventArray = realloc(eventArray,sizeof(struct calEvent)*numEvents);
				assert(eventArray);
				eventArray[numEvents-1] = event;
			}
		}
		qsort(eventArray,numEvents,sizeof(struct calEvent),compareTimes);
		int count = 0;
		for(int i = 0; i < numEvents; i++,count++){
			struct tm * tmTime = localtime(&eventArray[i].theTime);
			char timeStr[TIMEBUFFSIZE];
			strftime(timeStr,TIMEBUFFSIZE,"%Y-%b-%d %l:%M %p", tmTime);
			int ret = fprintf(txtfile,"%s: %s\n",timeStr,eventArray[i].summary);
			status.code = checkIOERR(ret);
			if(status.code != OK){
				status.linefrom = count;
				status.lineto = count;
				return status;
			}
		}
		status.linefrom = numEvents;
		status.lineto = numEvents;

	}
	else if(kind == OPROP){//handling X-PROPERTIES
		char ** propArray;//array to hold X-Properties
		int propLength = 0;//;ength of property array
		propArray = malloc(sizeof(char*)*propLength);
		assert(propArray);
		CalProp * current;
		current = comp->prop;

		while(current != NULL){
			if(strstr(current->name,"X-")){
				if(checkDuplicateXProp(propArray,propLength,current->name)== 0){
					propLength = propLength+1;
					propArray = realloc(propArray,(sizeof(char*)*(propLength)));
					assert(propArray);
					propArray[propLength-1] = malloc(sizeof(char*)*(strlen(current->name)+1));
					strcpy(propArray[propLength-1],current->name);
				}
			}
			current = current->next;
		}
		for(int i = 0; i < comp->ncomps;i++){
			CalProp * current;
			current = comp->comp[i]->prop;

			while(current != NULL){
				if(strstr(current->name,"X-")){
					if(checkDuplicateXProp(propArray,propLength,current->name)== 0){
						propLength = propLength+1;
						propArray = realloc(propArray,(sizeof(char*)*(propLength)));
						assert(propArray);
						propArray[propLength-1] = malloc(sizeof(char*)*(strlen(current->name)+1));
						strcpy(propArray[propLength-1],current->name);

					}
				}
				current = current->next;
			}
			for(int o = 0; o < comp->comp[i]->ncomps;o++){
				CalProp * current;
				current = comp->comp[i]->comp[o]->prop;

				while(current != NULL){
					if(strstr(current->name,"X-")){
						if(checkDuplicateXProp(propArray,propLength,current->name)== 0){
							propLength = propLength+1;
							propArray = realloc(propArray,(sizeof(char*)*(propLength)));
							assert(propArray);
							propArray[propLength-1] = malloc(sizeof(char*)*(strlen(current->name)+1));
							strcpy(propArray[propLength-1],current->name);

						}
					}
					current = current->next;
				}
			}
		}
		qsort(propArray, propLength, sizeof(char*), compareNames);
		int count = 0;
		for(int i = 0; i < propLength; i++,count++){
			int ret = fprintf(txtfile,"%s\n",propArray[i]);
			status.code = checkIOERR(ret);
			if(status.code != OK){
				status.linefrom = count;
				status.lineto = count;
				return status;
			}
		}
		status.linefrom = propLength;
		status.lineto = propLength;

		


	}
	status.code = OK;
	return status;

}
/*removeComponent
* Arguments: index to remove, calendar struct
* Description: loops to the desired index and removes it from the calendar
*/
void removeComponent(int index, CalComp * comp){
	
	for(int i = index; i < comp->ncomps-1; i++){
		comp->comp[i] = comp->comp[i+1];
	}
	comp->ncomps--;
}
/*checkValidDates
* Arguments: the from and to dates,calcomp struct
* Return: 0 if not valid, 1 if vaid
* Description: searches the properties for any dates that come between the range
*/
int checkValidDates(time_t datefrom, time_t dateto,CalComp * comp){
	CalProp * current = comp->prop;

	int found = 0;//integer representing of any times were found or not

	while(current != NULL){
		if(strcmp(current->name,"COMPLETED" )==0){//checks each type of TIME, and compares
			struct tm * tm = malloc(sizeof(struct tm));
			tm->tm_isdst = -1;
			assert(tm!=NULL);

			strptime(current->value, "%Y%m%dT%H%M%S", tm);
			time_t timeNum = mktime(tm);

			if(timeNum >= datefrom && timeNum <= dateto){///if the time is between the range
				found = 1;
			}

			free(tm);
		}
		else if(strcmp(current->name,"DTEND" )==0){
			struct tm * tm = malloc(sizeof(struct tm));
			tm->tm_isdst = -1;
			assert(tm!=NULL);

			strptime(current->value, "%Y%m%dT%H%M%S", tm);
			tm->tm_isdst = -1;
			time_t timeNum = mktime(tm);

			if(timeNum >= datefrom && timeNum <= dateto){
				found = 1;
			}


			free(tm);
		}
		else if(strcmp(current->name,"DUE" )==0){
			struct tm * tm = malloc(sizeof(struct tm));
			tm->tm_isdst = -1;
			assert(tm!=NULL);

			strptime(current->value, "%Y%m%dT%H%M%S", tm);
			time_t timeNum = mktime(tm);

			if(timeNum >= datefrom && timeNum <= dateto){
				found = 1;
			}

			free(tm);

		}
		else if(strcmp(current->name,"DTSTART" )==0){
			struct tm * tm = malloc(sizeof(struct tm));
			tm->tm_isdst = -1;
			assert(tm!=NULL);

			strptime(current->value, "%Y%m%dT%H%M%S", tm);
			time_t timeNum = mktime(tm);

			if(timeNum >= datefrom && timeNum <= dateto){
				found = 1;
			}

			free(tm);
		}
		current = current->next;
	}
	if(found == 0){//if none were found, then recursively check the subcomponents
		for(int i = 0; i < comp->ncomps; i++){
			found = checkValidDates(datefrom,dateto,comp->comp[i]);
			if(found == 1){
				return found;
			}
		}
	}
	return found;
}
/*checkBeforeDate
* Arguments: the to dates,calcomp struct
* Return: 0 if not valid, 1 if vaid
* Description: searches the properties for any dates that come before the specified date
*/
int checkBeforeDate(time_t dateto, CalComp * comp){
	CalProp * current = comp->prop;

	int found = 0;

	while(current != NULL){//checks each type of TIME, and compares
		if(strcmp(current->name,"COMPLETED" )==0){
			struct tm * tm = malloc(sizeof(struct tm));
			tm->tm_isdst = -1;
			assert(tm!=NULL);

			strptime(current->value, "%Y%m%dT%H%M%S", tm);
			time_t timeNum = mktime(tm);

			if(timeNum <= dateto){
				found = 1;
			}

			free(tm);
		}
		else if(strcmp(current->name,"DTEND" )==0){
			struct tm * tm = malloc(sizeof(struct tm));
			tm->tm_isdst = -1;
			assert(tm!=NULL);

			strptime(current->value, "%Y%m%dT%H%M%S", tm);
			tm->tm_isdst = -1;
			time_t timeNum = mktime(tm);

			if(timeNum <= dateto){
				found = 1;
			}


			free(tm);
		}
		else if(strcmp(current->name,"DUE" )==0){
			struct tm * tm = malloc(sizeof(struct tm));
			tm->tm_isdst = -1;
			assert(tm!=NULL);

			strptime(current->value, "%Y%m%dT%H%M%S", tm);
			time_t timeNum = mktime(tm);

			if(timeNum <= dateto){
				found = 1;
			}

			free(tm);

		}
		else if(strcmp(current->name,"DTSTART" )==0){
			struct tm * tm = malloc(sizeof(struct tm));
			tm->tm_isdst = -1;
			assert(tm!=NULL);

			strptime(current->value, "%Y%m%dT%H%M%S", tm);
			time_t timeNum = mktime(tm);

			if(timeNum <= dateto){
				found = 1;
			}

			free(tm);
		}
		current = current->next;
	}
	if(found == 0){
		for(int i = 0; i < comp->ncomps; i++){
			found = checkBeforeDate(dateto,comp->comp[i]);
			if(found == 1){
				return found;
			}
		}
	}
	return found;

}
/*checkAfterDate
* Arguments: the from dates,calcomp struct
* Return: 0 if not valid, 1 if vaid
* Description: searches the properties for any dates that come after the specified date
*/
int checkAfterDate(time_t datefrom, CalComp * comp){
	CalProp * current = comp->prop;

	int found = 0;

	while(current != NULL){
		if(strcmp(current->name,"COMPLETED" )==0){
			struct tm * tm = malloc(sizeof(struct tm));
			tm->tm_isdst = -1;
			assert(tm!=NULL);

			strptime(current->value, "%Y%m%dT%H%M%S", tm);
			time_t timeNum = mktime(tm);

			if(timeNum >= datefrom){
				found = 1;
			}

			free(tm);
		}
		else if(strcmp(current->name,"DTEND" )==0){
			struct tm * tm = malloc(sizeof(struct tm));
			tm->tm_isdst = -1;
			assert(tm!=NULL);

			strptime(current->value, "%Y%m%dT%H%M%S", tm);
			tm->tm_isdst = -1;
			time_t timeNum = mktime(tm);

			if(timeNum >= datefrom){
				found = 1;
			}


			free(tm);
		}
		else if(strcmp(current->name,"DUE" )==0){
			struct tm * tm = malloc(sizeof(struct tm));
			tm->tm_isdst = -1;
			assert(tm!=NULL);

			strptime(current->value, "%Y%m%dT%H%M%S", tm);
			time_t timeNum = mktime(tm);

			if(timeNum >= datefrom){
				found = 1;
			}

			free(tm);

		}
		else if(strcmp(current->name,"DTSTART" )==0){
			struct tm * tm = malloc(sizeof(struct tm));
			tm->tm_isdst = -1;
			assert(tm!=NULL);

			strptime(current->value, "%Y%m%dT%H%M%S", tm);
			time_t timeNum = mktime(tm);

			if(timeNum >= datefrom){
				found = 1;
			}

			free(tm);
		}
		current = current->next;
	}
	if(found == 0){
		for(int i = 0; i < comp->ncomps; i++){
			found = checkAfterDate(datefrom,comp->comp[i]);
			if(found == 1){
				return found;
			}
		}
	}
	return found;
	
}
CalStatus calFilter(const CalComp *comp, CalOpt content, time_t datefrom, time_t dateto, FILE *const icsfile){
	
	CalStatus status;
	if(content == OEVENT){//filtering events
		CalComp * copy = malloc(sizeof(CalComp)+(comp)->ncomps*sizeof(CalComp*));
		assert(copy);
		copy = memcpy(copy,comp,sizeof(CalComp) + (comp)->ncomps*sizeof(CalComp*));//shallow copy

		if(datefrom == 0 && dateto == 0){//no dates input, filtering by event
			for(int i = 0; i < copy->ncomps; i++){
				if(strcmp(copy->comp[i]->name,"VEVENT")!=0){
					removeComponent(i,copy);
					i--;
				}
			}
		}
		else if(datefrom != 0 && dateto != 0){//both dates are entered
			for(int i = 0; i < copy->ncomps; i++){
				if(strcmp(copy->comp[i]->name,"VEVENT")!=0){
					removeComponent(i,copy);
					i--;
				}
				else if(strcmp(copy->comp[i]->name,"VEVENT")==0){
					if(checkValidDates(datefrom,dateto,copy->comp[i])!=1){
						removeComponent(i,copy);
						i--;
					}
				}
			}
		}
		else if(dateto != 0){//only to date
			for(int i = 0; i < copy->ncomps; i++){
				if(strcmp(copy->comp[i]->name,"VEVENT")!=0){
					removeComponent(i,copy);
					i--;
				}
				else if(strcmp(copy->comp[i]->name,"VEVENT")==0){
					if(checkBeforeDate(dateto,copy->comp[i])!=1){
						removeComponent(i,copy);
						i--;
					}
				}
			}
		}
		else if(datefrom != 0){//only from date
			for(int i = 0; i < copy->ncomps; i++){
				if(strcmp(copy->comp[i]->name,"VEVENT")!=0){
					removeComponent(i,copy);
					i--;
				}
				else if(strcmp(copy->comp[i]->name,"VEVENT")==0){
					if(checkAfterDate(datefrom,copy->comp[i])!=1){
						removeComponent(i,copy);
						i--;
					}
				}
			}
		}
		if(copy->ncomps == 0){//if the filtered calendar has no components, NOCAL is returned
			status.code = NOCAL;
			status.lineto = 0;
			status.linefrom = 0;
			return status;
		}
		status = writeCalComp(icsfile,copy);
		free(copy);

	}
	else if(content == OTODO){//filtering TODO, same as Events
		CalComp * copy = malloc(sizeof(CalComp)+(comp)->ncomps*sizeof(CalComp*));
		assert(copy);
		copy = memcpy(copy,comp,sizeof(CalComp) + (comp)->ncomps*sizeof(CalComp*));
		if(datefrom == 0 && dateto == 0){//no dates input, filtering by event
			for(int i = 0; i < copy->ncomps; i++){
				if(strcmp(copy->comp[i]->name,"VTODO")!=0){
					removeComponent(i,copy);
					i--;
				}
			}
		}
		else if(datefrom != 0 && dateto != 0){
			for(int i = 0; i < copy->ncomps; i++){
				if(strcmp(copy->comp[i]->name,"VTODO")!=0){
					removeComponent(i,copy);
					i--;
				}
				else if(strcmp(copy->comp[i]->name,"VTODO")==0){
					if(checkValidDates(datefrom,dateto,copy->comp[i])!=1){
						removeComponent(i,copy);
						i--;
					}
				}
			}
		}
		else if(dateto != 0){
			for(int i = 0; i < copy->ncomps; i++){
				if(strcmp(copy->comp[i]->name,"VTODO")!=0){
					removeComponent(i,copy);
					i--;
				}
				else if(strcmp(copy->comp[i]->name,"VTODO")==0){
					if(checkBeforeDate(dateto,copy->comp[i])!=1){
						removeComponent(i,copy);
						i--;
					}
				}
			}
		}
		else if(datefrom != 0){
			for(int i = 0; i < copy->ncomps; i++){
				if(strcmp(copy->comp[i]->name,"VTODO")!=0){
					removeComponent(i,copy);
					i--;
				}
				else if(strcmp(copy->comp[i]->name,"VTODO")==0){
					if(checkAfterDate(datefrom,copy->comp[i])!=1){
						removeComponent(i,copy);
						i--;
					}
				}
			}
		}

		if(copy->ncomps == 0){
			status.code = NOCAL;
			status.lineto = 0;
			status.linefrom = 0;

			return status;
		}
		status = writeCalComp(icsfile,copy);
		free(copy);

	}

	return status;

}
/*removeVersion
* Arguments: pointer to head of prop lst
* Description: removes VERSION property from the list
*/
void removeVersion (CalProp * head){
	CalProp * current;
	CalProp * temp;
	current = head;

	if(strcmp(current->name,"VERSION")==0){
		temp = current;
		current = temp->next;
	}
	else{
		while(strcmp(current->next->name,"VERSION")!=0){
			current = current->next;
		}
		temp = current->next;
		current->next = temp->next;
	}
	current = head;
}
/*removeVersion
* Arguments: pointer to head of prop lst
* Description: removes PRODID property from the list
*/
void removeProdid(CalProp * head){
	CalProp * current;
	CalProp * temp;
	current = head;

	if(strcmp(current->name,"PRODID")==0){
		temp = current;
		current = temp->next;
	}
	else{
		while(strcmp(current->next->name,"PRODID")!=0){
			current = current->next;
		}
		temp = current->next;
		current->next = temp->next;
	}
	current = head;


}
CalStatus calCombine( const CalComp *comp1, const CalComp *comp2, FILE *const icsfile ){
	CalStatus status;
        status.code = OK;
	CalComp * copy = malloc(sizeof(CalComp)+(comp1)->ncomps*sizeof(CalComp*));
	assert(copy);
	copy = memcpy(copy,comp1,sizeof(CalComp) + (comp1)->ncomps*sizeof(CalComp*));//shallow copy
	CalProp * current;

	
	current = copy->prop;//iterating through prop list
	while(current->next != NULL){
		current = current->next;
	}
	if((strcmp(comp2->prop->name,"VERSION")==0 ||strcmp(comp2->prop->name,"PRODID")==0) && 
		(strcmp(comp2->prop->next->name,"VERSION")==0 
			||strcmp(comp2->prop->next->name,"PRODID")==0)){
		//if it begins with either VERSION or PRODID and the other one follows
		current->next = comp2->prop->next->next;
}
else if((strcmp(comp2->prop->name,"VERSION")==0 ||strcmp(comp2->prop->name,"PRODID")==0)){
		//if only one is at the beginning
	current->next = comp2->prop->next;
}
else{
	current->next = comp2->prop;
}

removeProdid(comp2->prop);
removeVersion(comp2->prop);

int comp1Length = comp1->ncomps;
int comp2Length = comp2->ncomps;
int newncomp = comp1Length + comp2Length;
copy = realloc(copy,(sizeof(CalComp) + newncomp*sizeof(CalComp*)));

	//moving all of comp1's comps into copy
for(int i = 0; i < comp1Length;i++){
	copy->comp[i] = comp1->comp[i];
}
int k = 0;
	//moving the rest of comp2s into copy
for(int j = comp1Length; j < newncomp ; j++,k++){
	copy->comp[j] = comp2->comp[k];
}
copy->ncomps = newncomp;

status = writeCalComp(icsfile,copy);
free(copy);

return status;

}



int main(int argc,char *argv[]){
	CalStatus status;
	if(argc == 1){
		fprintf(stderr,"Please provide arguments\n");
		return EXIT_FAILURE;
	}
	if(strcmp(argv[1],"-info")==0){//user selects info
		if(argc != 2){
			fprintf(stderr,"Input Error, Syntax: ./caltool -info\n");
                        return EXIT_FAILURE;
		}
		FILE * ics = stdin;
		FILE * outfile = stdout;
		CalComp * cal;
		status = readCalFile(ics,&cal);
		if(status.code != OK){
			fprintf(stderr,"Error: ");
			printExitStatus(status,stderr);
			return EXIT_FAILURE;
		}
		status = calInfo(cal,status.lineto,outfile);
		if(status.code != OK){
			fprintf(stderr,"Error: ");
			printExitStatus(status,stderr);
			return EXIT_FAILURE;
		}
		freeCalComp(cal);

	}
	else if(strcmp(argv[1],"-extract")==0){//user selects extract
		CalOpt option;
		if(argc != 3){
			fprintf(stderr,"Input Error, Syntax: ./caltool -extract <e or x>\n");
			return EXIT_FAILURE;
		}
		
		if(strcasecmp(argv[2],"e")==0){
			option = OEVENT;
		}
		else if(strcasecmp(argv[2],"x")==0){
			option = OPROP;
		}
		else{
			fprintf(stderr,"Incorrect type (require 'e' or 'x'\n");
				return EXIT_FAILURE;
			}
			FILE * ics = stdin;
			FILE * outfile = stdout;
			CalComp * cal;
			status = readCalFile(ics,&cal);
			if(status.code != OK){
				fprintf(stderr,"Error: ");
				printExitStatus(status,stderr);
				return EXIT_FAILURE;
			}
			status = calExtract(cal,option,outfile);
			if(status.code != OK){
				fprintf(stderr,"Error: ");
				printExitStatus(status,stderr);
				return EXIT_FAILURE;
			}
		}
		else if(strcmp(argv[1],"-filter")==0){
			CalOpt option;
			time_t dateFrom;
			time_t dateTo;

		if(argc == 3){//nodates
			dateFrom = 0;
			dateTo = 0;
			if(strcmp(argv[2],"e")==0){
				option = OEVENT;
			}
			else if(strcmp(argv[2],"t")==0){
				option = OTODO;
			}
		}
		else if(argc == 5){// one date
			if(strcmp(argv[2],"e")==0){
				option = OEVENT;
			}
			else if(strcmp(argv[2],"t")==0){
				option = OTODO;
			}
			if(strcasecmp(argv[3],"from")==0){
				int ret;
				struct tm*tmStruct = malloc(sizeof(struct tm));
				assert(tmStruct);


				if(strcmp(argv[4],"today")==0){
					dateTo = time(NULL);
					tmStruct = localtime(&dateFrom);
					tmStruct->tm_hour = 0;
					tmStruct->tm_min = 0;
					tmStruct->tm_sec = 0;
					tmStruct->tm_isdst = -1;

				}
				else{
					ret = getdate_r(argv[4],tmStruct);
					if(ret >= 1 && ret <= 5){
						fprintf(stderr,"Problem with DATEMSK environment variable or template file\n");
						return EXIT_FAILURE;
					}
					else if(ret >= 7){
						fprintf(stderr,"Date \"%s\" could not be interpreted\n",argv[4]);
						return EXIT_FAILURE;
					}
					tmStruct->tm_hour = 0;
					tmStruct->tm_min = 0;
					tmStruct->tm_sec = 0;
					tmStruct->tm_isdst = -1;
				}

				dateFrom = mktime(tmStruct);
				dateTo = 0;
				
				//if it's "today", substitute it for current date
				
			}
			else if(strcasecmp(argv[3],"to")==0){
				int ret;
				struct tm*tmStruct = malloc(sizeof(struct tm));
				assert(tmStruct);
				if(strcmp(argv[4],"today")==0){
					dateTo = time(NULL);
					tmStruct = localtime(&dateFrom);
					tmStruct->tm_hour = 0;
					tmStruct->tm_min = 0;
					tmStruct->tm_sec = 0;
					tmStruct->tm_isdst = -1;

				}
				else{
					ret = getdate_r(argv[4],tmStruct);
					if(ret >= 1 && ret <= 5){
						fprintf(stderr,"Problem with DATEMSK environment variable or template file\n");
						return EXIT_FAILURE;
					}
					else if(ret >= 7){
						fprintf(stderr,"Date \"%s\" could not be interpreted\n",argv[4]);
						return EXIT_FAILURE;
					}
					tmStruct->tm_hour = 23;
					tmStruct->tm_min = 59;
					tmStruct->tm_sec = 0;
					tmStruct->tm_isdst = -1;
				}
				

				dateTo = mktime(tmStruct);
				dateFrom = 0;
				//if it's "today, substitute for current date"
			}

		}
		else if(argc == 7){//to and from date

			int ret;
			struct tm*tmStruct = malloc(sizeof(struct tm));
			assert(tmStruct);

			if(strcmp(argv[2],"e")==0){
				option = OEVENT;
			}
			else if(strcmp(argv[2],"t")==0){
				option = OTODO;
			}

			if(strcmp(argv[4],"today")==0){
				dateFrom = time(NULL);
				tmStruct = localtime(&dateFrom);
				tmStruct->tm_hour = 0;
				tmStruct->tm_min = 0;
				tmStruct->tm_sec = 0;
				tmStruct->tm_isdst = -1;

			}
			else{
				ret = getdate_r(argv[4],tmStruct);
				if(ret >= 1 && ret <= 5){
					fprintf(stderr,"Problem with DATEMSK environment variable or template file\n");
					return EXIT_FAILURE;
				}
				else if(ret >= 7){
					fprintf(stderr,"Date \"%s\" could not be interpreted\n",argv[4]);
					return EXIT_FAILURE;
				}
				tmStruct->tm_hour = 0;
				tmStruct->tm_min = 0;
				tmStruct->tm_sec = 0;
				tmStruct->tm_isdst = -1;

			}

			dateFrom = mktime(tmStruct);
			
			

			int ret2;
			struct tm*tmStruct2 = malloc(sizeof(struct tm));
			assert(tmStruct2);
			if(strcmp(argv[6],"today")==0){
				dateTo = time(NULL);
				tmStruct2 = localtime(&dateTo);
				tmStruct2->tm_hour = 23;
				tmStruct2->tm_min = 59;
				tmStruct2->tm_sec = 0;
				tmStruct2->tm_isdst = -1;

			}
			else{
				ret2 = getdate_r(argv[6],tmStruct2);
				if(ret2 >= 1 && ret2 <= 5){
					fprintf(stderr,"Problem with DATEMSK environment variable or template file\n");
					return EXIT_FAILURE;
				}
				else if(ret2 >= 7){
					fprintf(stderr,"Date \"%s\" could not be interpreted\n",argv[6]);
					return EXIT_FAILURE;
				}
				tmStruct2->tm_hour = 23;
				tmStruct2->tm_min = 59;
				tmStruct2->tm_sec = 0;
				tmStruct2->tm_isdst = -1;
			}


			dateTo = mktime(tmStruct2);

			//check if it's today

			//if from is later than to, print error
			if(dateFrom - dateTo > 0){
				fprintf(stderr,"Error: From date is later than the to date\n");
				return EXIT_FAILURE;
			}
		}
		else{
			fprintf(stderr,"Insufficient arguments, Syntax: ./caltool -filter <e/t> from <date> to <date>\nOr:\n./caltool -filter <e/t>\n");
			return EXIT_FAILURE;

		}
		FILE * ics = stdin;
		FILE * outfile = stdout;
		CalComp * cal;
		status = readCalFile(ics,&cal);
		if(status.code != OK){
			fprintf(stderr,"Error: ");
			printExitStatus(status,stderr);
			return EXIT_FAILURE;
		}
		status = calFilter(cal,option,dateFrom,dateTo,outfile);
		if(status.code != OK){
			fprintf(stderr,"Error: ");
			printExitStatus(status,stderr);
			return EXIT_FAILURE;
		}
		freeCalComp(cal);
	}
	else if(strcmp(argv[1],"-combine")==0){//if the user selects "combine"
		if(argc != 3){
			fprintf(stderr,"Input Error, Syntax: ./caltool -combine <filename.ics>\n");
			return EXIT_FAILURE;
		}
		FILE * tocombine = fopen(argv[2],"r");
		if(tocombine == NULL){
			fprintf(stderr,"Error opening file\n");
			return EXIT_FAILURE;
		}
		
		FILE * ics = stdin;
		CalComp * cal;
		CalComp * calToCombine;
		status = readCalFile(ics,&cal);
		if(status.code != OK){
			fprintf(stderr,"Error: ");
			printExitStatus(status,stderr);
			return EXIT_FAILURE;
		}
		status = readCalFile(tocombine,&calToCombine);
		if(status.code != OK){
			fprintf(stderr,"Error: ");
			printExitStatus(status,stderr);
			return EXIT_FAILURE;
		}
		status = calCombine(cal,calToCombine,stdout);
		if(status.code != OK){
			fprintf(stderr,"Error: ");
			printExitStatus(status,stderr);
			return EXIT_FAILURE;
		}
		freeCalComp(cal);
		freeCalComp(calToCombine);

	}
	else{
		fprintf(stderr,"Please enter a valid command (info,extract,filter,combine)\n");
		return EXIT_FAILURE;
	}
	

	return EXIT_SUCCESS;
}

/*
# Copyright (c) Members of the EGEE Collaboration. 2009. 
# See http://www.eu-egee.org/partners/ for details on the copyright
# holders.  
# 
# Licensed under the Apache License, Version 2.0 (the "License"); 
# you may not use this file except in compliance with the License. 
# You may obtain a copy of the License at 
# 
#     http://www.apache.org/licenses/LICENSE-2.0 
# 
# Unless required by applicable law or agreed to in writing, software 
# distributed under the License is distributed on an "AS IS" BASIS, 
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
# See the License for the specific language governing permissions and 
# limitations under the License.
#
*/

#include "BUpdaterSGE.h"

extern int bfunctions_poll_timeout;

int main(int argc, char *argv[]){

	FILE *fd;
	job_registry_entry *en;
	time_t now;
	time_t purge_time=0;
	char *constraint=NULL;
	char *query=NULL;
	char *q=NULL;
	char *pidfile=NULL;
        char *final_string=NULL;
        char *cp=NULL;
        char *tpath;
	
	int version=0;
	int first=TRUE;
        int tmptim;
        int finstr_len=0;
        int loop_interval=DEFAULT_LOOP_INTERVAL;
	
        int fsq_ret=0;

	int c;				

	int confirm_time=0;

        static int help;
        static int short_help;
		
	while (1) {
		static struct option long_options[] =
		{
		{"help",      no_argument,     &help,       1},
		{"usage",     no_argument,     &short_help, 1},
		{"nodaemon",  no_argument,       0, 'o'},
		{"version",   no_argument,       0, 'v'},
		{0, 0, 0, 0}
		};

		int option_index = 0;
     
		c = getopt_long (argc, argv, "vo",long_options, &option_index);
     
		if (c == -1){
			break;
		}
     
		switch (c)
		{

		case 0:
		if (long_options[option_index].flag != 0){
			break;
		}
     
		case 'v':
			version=1;
			break;
	       
		case 'o':
			nodmn=1;
			break;

		case '?':
			break;
     
		default:
			abort ();
		}
	}

	if(help){
		usage();
	}
	 
	if(short_help){
		short_usage();
	}
	
	argv0 = argv[0];

        signal(SIGHUP,sighup);
	
	if(version) {
		printf("%s Version: %s\n",progname,VERSION);
		exit(EXIT_SUCCESS);
	}  

        /* Checking configuration */
        check_config_file("UPDATER"); 

	cha = config_read(NULL);
	if (cha == NULL)
	{
		fprintf(stderr,"Error reading config: ");
		perror("");
		return -1;
	}

        ret = config_get("bupdater_child_poll_timeout",cha);
        if (ret != NULL){
                tmptim=atoi(ret->value);
                if (tmptim > 0) bfunctions_poll_timeout = tmptim*1000;
        }
	
        ret = config_get("bupdater_debug_level",cha);
        if (ret != NULL){
                debug=atoi(ret->value);
        }

        ret = config_get("bupdater_debug_logfile",cha);
        if (ret != NULL){
                debuglogname=strdup(ret->value);
                if(debuglogname == NULL){
                        sysfatal("strdup failed for debuglogname in main: %r");
                }
        }
        if(debug <=0){
	        debug=0;
	}

        if(debuglogname){
                if((debuglogfile = fopen(debuglogname, "a+"))==0){
                        debug = 0;
                }
        }else{
                debug = 0;
        }
        
	ret = config_get("debug_level",cha);
	if (ret != NULL){
		debug=atoi(ret->value);
	}
	
	ret = config_get("debug_logfile",cha);
	if (ret != NULL){
		debuglogname=strdup(ret->value);
                if(debuglogname == NULL){
                        sysfatal("strdup failed for debuglogname in main: %r");
                }
	}
	if(debug <=0){
		debug=0;
	}
    
	if(debuglogname){
		if((debuglogfile = fopen(debuglogname, "a+"))==0){
			debug = 0;
		}
	}else{
		debug = 0;
	}

        ret = config_get("sge_binpath",cha);
        if (ret == NULL){
                do_log(debuglogfile, debug, 1, "%s: key sge_binpath not found\n",argv0);
        } else {
                sge_binpath=strdup(ret->value);
                if(sge_binpath == NULL){
                        sysfatal("strdup failed for sge_binpath in main: %r");
                }
        }

        ret = config_get("sge_rootpath",cha);
        if (ret == NULL){
                do_log(debuglogfile, debug, 1, "%s: key sge_rootpath not found\n",argv0);
        } else {
                sge_rootpath=strdup(ret->value);
                if(sge_rootpath == NULL){
                        sysfatal("strdup failed for sge_rootpath in main: %r");
                }

                tpath=make_message("%s",sge_rootpath);
                if (opendir(tpath)==NULL){
                        do_log(debuglogfile, debug, 1, "%s: dir %s does not exist or is not readable\n",argv0,tpath);
                        sysfatal("dir %s does not exist or is not readable: %r",tpath);
                }
                free(tpath);
        }

        ret = config_get("sge_cellname",cha);
        if (ret == NULL){
                do_log(debuglogfile, debug, 1, "%s: key sge_cellname not found\n",argv0);
        } else {
                sge_cellname=strdup(ret->value);
                if(sge_cellname == NULL){
                        sysfatal("strdup failed for sge_cellname in main: %r");
                }
        }

        ret = config_get("sge_helperpath",cha);
        if (ret == NULL){
                do_log(debuglogfile, debug, 1, "%s: key sge_helperpath not found\n",argv0);
        } else {
                sge_helperpath=strdup(ret->value);
                if(sge_helperpath == NULL){
                        sysfatal("strdup failed for sge_helperpath in main: %r");
                }
        }
        ret = config_get("sge_helperpath",cha);
        if (ret == NULL){
                if(debug){
			fprintf(debuglogfile, "%s: key sge_helperpath not found\n",argv0);
			fflush(debuglogfile);
		}
        } else {
                sge_helperpath=strdup(ret->value);
                if(sge_helperpath == NULL){
                        sysfatal("strdup failed for sge_helperpath in main: %r");
                }
        }

        ret = config_get("sge_rootpath",cha);
        if (ret == NULL){
                if(debug){
			fprintf(debuglogfile, "%s: key sge_rootpath not found\n",argv0);
			fflush(debuglogfile);
		}
        } else {
                sge_rootpath=strdup(ret->value);
                if(sge_rootpath == NULL){
                        sysfatal("strdup failed for sge_rootpath in main: %r");
                }
        }
	
        ret = config_get("sge_cellname",cha);
        if (ret == NULL){
                if(debug){
			fprintf(debuglogfile, "%s: key sge_cellname not found\n",argv0);
			fflush(debuglogfile);
		}
        } else {
                sge_cellname=strdup(ret->value);
                if(sge_cellname == NULL){
                        sysfatal("strdup failed for sge_cellname in main: %r");
                }
        }
	
        ret = config_get("job_registry",cha);
        if (ret == NULL){
                do_log(debuglogfile, debug, 1, "%s: key job_registry not found\n",argv0);
                sysfatal("job_registry not defined. Exiting");
        } else {
                registry_file=strdup(ret->value);
                if(registry_file == NULL){
                        sysfatal("strdup failed for registry_file in main: %r");
                }
        }
	
        ret = config_get("purge_interval",cha);
        if (ret == NULL){
                do_log(debuglogfile, debug, 1, "%s: key purge_interval not found using the default:%d\n",argv0,purge_interval);
        } else {
                purge_interval=atoi(ret->value);
        }

	ret = config_get("finalstate_query_interval",cha);
        if (ret == NULL){
                do_log(debuglogfile, debug, 1, "%s: key finalstate_query_interval not found using the default:%d\n",argv0,finalstate_query_interval);
        } else {
                finalstate_query_interval=atoi(ret->value);
        }

       ret = config_get("alldone_interval",cha);
        if (ret == NULL){
                do_log(debuglogfile, debug, 1, "%s: key alldone_interval not found using the default:%d\n",argv0,alldone_interval);
        } else {
                alldone_interval=atoi(ret->value);
        }
	
        ret = config_get("bupdater_loop_interval",cha);
        if (ret == NULL){
                do_log(debuglogfile, debug, 1, "%s: key bupdater_loop_interval not found using the default:%d\n",argv0,loop_interval);
        } else {
                loop_interval=atoi(ret->value);
        }
	
        ret = config_get("bupdater_pidfile",cha);
        if (ret == NULL){
                do_log(debuglogfile, debug, 1, "%s: key bupdater_pidfile not found\n",argv0);
        } else {
                pidfile=strdup(ret->value);
                if(pidfile == NULL){
                        sysfatal("strdup failed for pidfile in main: %r");
                }
        }	

	if( !nodmn ) daemonize();

	if( pidfile ){
		writepid(pidfile);
		free(pidfile);
	}
	
	config_free(cha);

	rha=job_registry_init(registry_file, BY_BATCH_ID);	
	if (rha == NULL){
		do_log(debuglogfile, debug, 1, "%s: Error initialising job registry %s\n",argv0,registry_file);
		fprintf(stderr,"%s: Error initialising job registry %s :",argv0,registry_file);
		perror("");
	}
	
	for(;;){
		/* Purge old entries from registry */
		now=time(0);
		if(now - purge_time > 86400){
			if(job_registry_purge(registry_file, now-purge_interval,0)<0){
				do_log(debuglogfile, debug, 1, "%s: Error purging job registry %s\n",argv0,registry_file);
                        	fprintf(stderr,"%s: Error purging job registry %s :",argv0,registry_file);
                        	perror("");
			}else{
				purge_time=time(0);
			}
		}

		IntStateQuery();
		fd = job_registry_open(rha, "r");

		if (fd == NULL)
		{
                        do_log(debuglogfile, debug, 1, "%s: Error opening job registry %s\n",argv0,registry_file);
			fprintf(stderr,"%s: Error opening job registry %s :",argv0,registry_file);
			perror("");
			sleep(loop_interval);
			continue;
		}
		if (job_registry_rdlock(rha, fd) < 0)
		{
                        do_log(debuglogfile, debug, 1, "%s: Error read locking job registry %s\n",argv0,registry_file);
			fprintf(stderr,"%s: Error read locking job registry %s :",argv0,registry_file);
			perror("");
                        sleep(loop_interval);
			continue;
		}
		job_registry_firstrec(rha,fd);
		fseek(fd,0L,SEEK_SET);
		if((constraint=calloc(STR_CHARS,1)) == 0){
			sysfatal("can't malloc constraint %r");
        	}
		if((query=calloc(STR_CHARS,1)) == 0){
			sysfatal("can't malloc query %r");
        	}


		query[0]=' ';
		first=TRUE;
		while ((en = job_registry_get_next(rha, fd)) != NULL)
		{
			if(((now - en->mdate) > finalstate_query_interval) && en->status!=3 && en->status!=4)
			{
				/* create the constraint that will be used in condor_history command in FinalStateQuery*/
				sprintf(constraint," %s.%s",en->batch_id,sge_cellname);
				
				q=realloc(query,strlen(query)+strlen(constraint)+1);
				if(q != NULL){
					query=q;
				}else{
					if(debug){
						fprintf(debuglogfile, "can't realloc query\n");
						fflush(debuglogfile);
					}
                			fprintf(stderr,"%s: can't realloc query: ",argv[0]);
					perror("");
					sleep(2);
					continue;			
				}
				strcat(query,constraint);
				runfinal=TRUE;
			}
			/* Assign Status=4 and ExitStatus=-1 to all entries that after alldone_interval are still not in a final state(3 or 4) */
			if((now - en->mdate > alldone_interval) && en->status!=3 && en->status!=4 && !runfinal)
			{
			  AssignFinalState(en->batch_id);	
			}
			
			free(en);
		}
		if(runfinal){
			FinalStateQuery(query);
			runfinal=FALSE;
		}
		free(constraint);		
		free(query);
		fclose(fd);		
		sleep(10); /* Was 2 seconds,  but that is a bit too frequent for running qstat etc */
	}
	
	job_registry_destroy(rha);
		
	return(0);	
}


int
IntStateQuery()
{

        char *command_string;

	if((command_string=calloc(STR_CHARS,1)) == 0){
		sysfatal("can't malloc command_string %r");
	}
	
	sprintf(command_string,"%s --qstat --sgeroot=%s --cell=%s --all", sge_helperpath, sge_rootpath, sge_cellname);
	
	StateQuery( command_string );
	free(command_string);
	return(0);
}

int
FinalStateQuery(char *query)
{
        char *command_string;
	FILE *file_output;
	char line[6];
	job_registry_entry en;
	time_t now;
	char *string_now=NULL;
	
	if((command_string=calloc(NUM_CHARS+strlen(query),1)) == 0){
		sysfatal("can't malloc command_string %r");
	}

	char *list=strtok(query, " \n" );
	while (list != NULL){
	  sprintf(command_string,"%s --qstat --sgeroot=%s --cell=%s --status %s",sge_helperpath, sge_rootpath, sge_cellname, list);
	  file_output = popen(command_string,"r");
  
	  if (file_output == NULL) return 0;

	  fgets( line,sizeof(line), file_output );
	  if (strcmp (line,"Error") == 0){
		  char *elto=strtok(list, " ." );
		  JOB_REGISTRY_ASSIGN_ENTRY(en.batch_id,elto);
		  en.status=atoi("3");
		  en.exitcode=atoi("3");
		  JOB_REGISTRY_ASSIGN_ENTRY(en.wn_addr,"\0");
		  JOB_REGISTRY_ASSIGN_ENTRY(en.exitreason,"reason=3");
		  now=time(0);
		  string_now=make_message("%d",now);
		  JOB_REGISTRY_ASSIGN_ENTRY(en.updater_info,string_now)
		  en.udate=now;
		  free(string_now);
		  if ((ret=job_registry_update(rha, &en)) < 0)
		    {
		      fprintf(stderr,"Update of record returns %d: ", ret);
		      perror("");
		    }
		  else
		    {
		      if (en.status == REMOVED || en.status == COMPLETED){
			job_registry_unlink_proxy(rha, &en);
		      }
		    }

	  }else{
	    sprintf(command_string,"%s --sgeroot=%s --cell=%s --qacct %s", sge_helperpath, sge_rootpath, sge_cellname, list);
	    StateQuery( command_string );
	  }
	  sprintf(command_string,"\0");
	  pclose( file_output );
	  list = strtok (NULL, " \n");
	}

	free(command_string);
	return 0;
}



int StateQuery(char *command_string)
{
/*
 sge_helper output for unfinished jobs:
 batch_id  status  exitcode   udate(timestamp_for_current_status) workernode
 22018     2       0          1202288920			  hostname

 Filled entries:
 batch_id
 status
 exitcode
 udate
 wn_addr
 
 Filled by suhmit script:
 blah_id 
 
 Unfilled entries:
 exitreason
*/
        FILE *file_output;
	char line[STR_CHARS];
	char *token[6];
	job_registry_entry en;
	int ret;
        time_t now;
        char *string_now=NULL;

	file_output = popen(command_string,"r");

        if (file_output == NULL){
	  return 0;
	}



	while ( fgets( line, sizeof( line ), file_output ) > 0 ) {
		int i;
	        token[0] = strtok( line, " \n" );
		for ( i = 1 ; i <= 5 && token[i-1] != NULL ; i++ ) {
		  token[i] = strtok( NULL, " \n" );
		}

		if ( token[5] && strcmp( token[5], "OK" ) == 0 ) {
		  JOB_REGISTRY_ASSIGN_ENTRY(en.batch_id,token[0]);
		  en.status=atoi(token[1]);
		  en.exitcode=atoi(token[2]);
		  en.udate=atoi(token[3]);
		  JOB_REGISTRY_ASSIGN_ENTRY(en.wn_addr,token[4]);
		  JOB_REGISTRY_ASSIGN_ENTRY(en.exitreason,"\0");
		  now=time(0);
		  string_now=make_message("%d",now);
		  JOB_REGISTRY_ASSIGN_ENTRY(en.updater_info,string_now)
		  free(string_now);
		  
		  if ((ret=job_registry_update(rha, &en)) < 0)
		    {
		      fprintf(stderr,"Update of record returns %d: ", ret);
		      perror("");
		    }
		  else
		    {
		      if (en.status == REMOVED || en.status == COMPLETED)
			job_registry_unlink_proxy(rha, &en);
		    }
		}
	}
	pclose( file_output );
 
	return(0);
}

int AssignFinalState(char *batchid){

	job_registry_entry en;
	int ret,i;
	time_t now;

	now=time(0);
	
	JOB_REGISTRY_ASSIGN_ENTRY(en.batch_id,batchid);
	en.status=COMPLETED;
	en.exitcode=-1;
	en.udate=now;
	JOB_REGISTRY_ASSIGN_ENTRY(en.wn_addr,"\0");
	JOB_REGISTRY_ASSIGN_ENTRY(en.exitreason,"\0");
		
	if ((ret=job_registry_update(rha, &en)) < 0)
	{
		fprintf(stderr,"Update of record %d returns %d: ",i,ret);
		perror("");
	}
	else job_registry_unlink_proxy(rha, &en);
	return(0);
}


void sighup()
{
        if(debug){
                fclose(debuglogfile);
                if((debuglogfile = fopen(debuglogname, "a+"))==0){
                        debug = 0;
                }
        }
}


int 
usage()
{
	printf("Usage: BUpdaterSGE [OPTION...]\n");
	printf("  -o, --nodaemon     do not run as daemon\n");
	printf("  -v, --version      print version and exit\n");
	printf("\n");
	printf("Help options:\n");
	printf("  -?, --help         Show this help message\n");
	printf("  --usage            Display brief usage message\n");
        printf("  --test            Display a error message\n");
	exit(EXIT_SUCCESS);
}

int 
short_usage()
{
	printf("Usage: BUpdaterSGE [-ov?] [-o|--nodaemon] [-v|--version] [-?|--help] [--usage]\n");
	exit(EXIT_SUCCESS);
}

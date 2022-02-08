#include "common.h"
#include "BoundedBuffer.h"
#include "Histogram.h"
#include "common.h"
#include "HistogramCollection.h"
#include "FIFOreqchannel.h"
#include <utility>
#include <sys/wait.h>>

using namespace std;

struct ResponseData
{
     int person;
     double ecg;
};

void patient_thread_function(int person, int datapoints, int ecg, BoundedBuffer*request_buffer)
{
    /* What will the patient threads do? */
    //Parameters- Everything for Data Message, The BoundedBuffer
    //Psuedo Code - Convert Parameters into Data Message, Convert DataMsg to char* , use BB.push to push to vector
            
            double t = 0;
            for(int i = 1; i<=datapoints; i++)
			{
				datamsg x(person, t, 1);
				(*request_buffer).push((char*)&x, sizeof(x)); 
				t+=0.004;
			}
            //MESSAGE_TYPE q = QUIT_MSG;
            //(*request_buffer).push((char*)&q, sizeof(MESSAGE_TYPE)); //Pushes Final Quit Message

           // cout<<"In PATIENT THREAD"<<endl;
     //Pass for specific number of second points
}

void worker_thread_function(FIFORequestChannel*newchan, BoundedBuffer*request_buffer, BoundedBuffer*response, string filename, int bufcap)
{
    //Might have to change MAX_MESSAGE to buffercap later on
    /*
		Functionality of the worker threads	
    */
   //Parameters- FifoChan, a new ResponseBuffer
   //Function - Pass to Channel, Put Data in ResponseBuffer as char*
    /*MESSAGE_TYPE m = NEWCHANNEL_MSG;
    chan->cwrite (&m, sizeof (m));
    char newchanname [100];
    chan->cread (newchanname, sizeof (newchanname));
    FIFORequestChannel*newchan = new FIFORequestChannel(newchanname, FIFORequestChannel::CLIENT_SIDE);*/
    //cout<<"In Worker Thread Statement"<<endl;
   while(true)
   {
       
       char value[8192];
       int length = (*request_buffer).pop(value, bufcap);
       MESSAGE_TYPE mS = *(MESSAGE_TYPE *) value;
       //cout<<"MESSAGE TYPE: "<<mS<<endl;
       if(mS == DATA_MSG)//This will handle data message
       {
                //Getting Thread from Request Buffer
                datamsg s = *(datamsg*)value;
               // cout<<"Person"<<d.person<<endl;
               // cout<<"ECG"<<d.ecgno<<endl;
               // cout<<"Time"<<d.seconds<<endl;
               /*datamsg s(0,0,0);
                int ecg = d.ecgno;
                int person = d.person;
                double seconds = d.ecgno;
                memset(&s, 0, sizeof(datamsg));
                s.ecgno = ecg;
                s.mtype = DATA_MSG;
                s.person = d.person;
                s.seconds = d.seconds;*/

                newchan->cwrite(&s, sizeof(s));
                double ecgvalue;
                newchan->cread(&ecgvalue, sizeof(double));
                ResponseData p{s.person, ecgvalue};
                (*response).push((char*)&p, sizeof(p));
                //Make sure that person and ecg value linked before pushing response buffer(Use C++ pair or custom struct)
                //cout<<"MESSAGE TYPE: "<<mS<<endl;
                
                
        }
        else if(mS == FILE_MSG)
        {
            
                    filemsg fmess = *(filemsg*)value;
                    string fname = filename;
                    string outputFilePath = "recieved/"+ fname;
                    FILE*outfile = fopen(outputFilePath.c_str(), "r+");
                    fseek(outfile, fmess.offset, SEEK_SET);
                    int len = sizeof (filemsg) + fname.size()+1;
                    char buf3 [len];
                    memcpy (buf3, &fmess, sizeof (filemsg));
                    strcpy (buf3 + sizeof (filemsg), fname.c_str());
                    buf3[len] = '\0';
                    newchan->cwrite (buf3, len);
                    
                    uint64_t resp[fmess.length];
                    int nbytes = newchan->cread(&resp, fmess.length);
                   // fseek(outfile, fmess.offset, SEEK_SET);
                    fwrite(resp, 1, fmess.length, outfile);
                    fclose(outfile);
            
        }
        else if(mS == QUIT_MSG)
        {
            //cout<<"In QUIT MSG"<<endl;
            newchan->cwrite((char*)&mS, sizeof(MESSAGE_TYPE));
            delete newchan;
            return;
        }
   
  }
    



   



}
void histogram_thread_function (BoundedBuffer*response, HistogramCollection * hc)
{
    /*
		Functionality of the histogram threads	
    */
    char value[8192];
    while(true)
    {
        
        int length = (*response).pop(value, MAX_MESSAGE);
        ResponseData d = *(ResponseData*)value;
        if(d.person < 0 || d.person > 15)
        {
            break;
        }
        hc->useHistogramUpdate(d.person, d.ecg);
    }

}

void file_request(string filename, __int64_t fLen, BoundedBuffer* request_buffer, int buffercap)
{
   

    //open file for fseek()
    FILE*fp;
    fp = fopen(("recieved/"+filename).c_str(), "w");
    fseek(fp, fLen, SEEK_SET);
    rewind(fp);
    fclose(fp);
    
    //cout<<"IN FILE REQUEST"<<endl;


    int fmParameterOne = 0;
	int fmParameterTwo = buffercap;
    int numIterations = ceil(fLen*1.0/buffercap);
			
			 
			for(int i = 0; i<numIterations; i++)
			{
				    //filemsg fmess(fmParameterOne, fmParameterTwo);
					if(fmParameterOne >= fLen)
					{
						break;
					}
					
					if(fLen - fmParameterOne < buffercap)
					{
						fmParameterTwo = fLen - fmParameterOne;
					}
					//string fname = "teslkansdlkjflasjdf.dat";
					filemsg fmess(fmParameterOne, fmParameterTwo);
                    (*request_buffer).push((char*)&fmess, buffercap);
			
					
                    fmParameterOne += buffercap;
			}
           // MESSAGE_TYPE q = QUIT_MSG;
           // (*request_buffer).push((char*)&q, MAX_MESSAGE);

}



int main(int argc, char *argv[])
{
    int opt;
    int n = 100;    		//default number of requests per "patient"
    int p = 1;     		// number of patients [1,15]
    int w = 1;    		//default number of worker threads
    int b = 20; 		// default capacity of the request buffer, you should change this default
	int m = MAX_MESSAGE; 	// default capacity of the message buffer
    int h = 0;              //Number of Histograms
    string filename = "";
    bool isFile = false;
    srand(time_t(NULL));
    
    while ((opt = getopt(argc, argv, "n:p:w:b:m:h:f:")) != -1) 
	{
		switch (opt) {
			case 'n':
				n = atoi (optarg);
				break;
			case 'p':
				p = atof (optarg);
				break;
			case 'w':
				w = atoi (optarg);
				break;
			case 'b':
				b = atoi(optarg);
				break;
			case 'h':
				h = atoi(optarg);
				break;
			case 'm':
			    m = atoi(optarg);
				break;
            case 'f':
			    filename = optarg;
                isFile = true;
				break;
		}
	}
    
    int pid = fork();
    if (pid == 0){
		// modify this to pass along m
        char*args[] = {"./server", "-m", (char*) to_string(m).c_str(), NULL};
		execvp(args[0], args);
    }
    
	FIFORequestChannel* chan = new FIFORequestChannel("control", FIFORequestChannel::CLIENT_SIDE);
    BoundedBuffer request_buffer(b);
	HistogramCollection hc;
    BoundedBuffer response(b);
	
     /*datamsg d(1, 2, 1);
     datamsg f(2, 2, 1);
     request_buffer.push((char*)&d, sizeof(d));
     request_buffer.push((char*)&f, sizeof(d));
     char v1[1024];
     char v2[1024];
     int l1 = request_buffer.pop(v1, MAX_MESSAGE);
     int l2 = request_buffer.pop(v2, MAX_MESSAGE);
     cout<<"L1   "<<l1<<endl;
     cout<<"L2   "<<l2<<endl;
     datamsg newD = *(datamsg*)v1;
     cout<<"PNEWD: "<<newD.person<<endl;
     cout<<"TNEWD: "<<newD.seconds<<endl;
     cout<<"ENEWD: "<<newD.ecgno<<endl;
     datamsg newF = *(datamsg*)v2;
     cout<<"PNEWD: "<<newF.person<<endl;
     cout<<"TNEWD: "<<newF.seconds<<endl;
     cout<<"ENEWD: "<<newF.ecgno<<endl;*/
	/*string channame = "";
    patient_thread_function(1, 2, 1, &request_buffer);
    MESSAGE_TYPE g = QUIT_MSG;
    request_buffer.push((char *) &g, sizeof (MESSAGE_TYPE));
    worker_thread_function(chan, &request_buffer, &response, filename);*/

    FIFORequestChannel*channels[w];
    for(int i = 0; i<w; i++)
    {
          MESSAGE_TYPE m = NEWCHANNEL_MSG;
          chan->cwrite (&m, sizeof (m));
          char newchanname [100];
          chan->cread (newchanname, sizeof (newchanname));
          FIFORequestChannel*newchan = new FIFORequestChannel(newchanname, FIFORequestChannel::CLIENT_SIDE);
          channels[i] = newchan;
    }
   
    for(int i = 0; i<p; i++)
    {
        Histogram*histo = new Histogram(10, -2.0, 2.0);
        hc.add(histo);
    }
	
    struct timeval start, end;
    gettimeofday (&start, 0);

    // Start all threads here
    thread fileThread; 
    thread patients[p];
    thread workers[w];
    thread histograms[h];

    if(isFile == true)
    {
        filemsg fm (0,0);
	    string fname = filename;
	
		int len = sizeof (filemsg) + fname.size()+1;
		char buf2 [len];
		memcpy (buf2, &fm, sizeof (filemsg));
		strcpy (buf2 + sizeof (filemsg), fname.c_str());
		chan->cwrite (buf2, len);  // I want the file length;
			
		__int64_t fLen; 
		__int64_t fileLength = chan->cread(&fLen, sizeof(__int64_t));

        fileThread = thread(file_request, fname, fLen, &request_buffer, m);

        //cout<<"FILE LENGTH "<<fLen<<endl;
    }
    else
    {
        for(int i = 0; i<p; i++)
        {
           patients[i] = thread(patient_thread_function, i+1, n, 1, &request_buffer);
        
        }
    }

    //Create Patients
   /* for(int i = 0; i<p; i++)
    {
        patients[i] = thread(patient_thread_function, i+1, n, 1, &request_buffer);
        
    }*/

    //Create Workers
    for(int i = 0; i<w; i++)
    {
        
        workers[i] = thread(worker_thread_function, channels[i], &request_buffer, &response, filename, m);
        
    }

    //Create Histograms
    for(int i = 0; i<h; i++)
    {
        histograms[i] = thread(histogram_thread_function,&response, &hc);
    }
	

	/* Join all threads here */
    if(isFile == true)
    {
        fileThread.join();
    }
    else
    {
        for(int i = 0; i<p; i++)
        {
           patients[i].join();
        }
    }
    //Join Patient Threads
    /*for(int i = 0; i<p; i++)
    {
        patients[i].join();
    }*/

    //Send Quit Messages for Worker
    for(int i = 0; i<w; i++)
    {
        MESSAGE_TYPE q = QUIT_MSG;
        request_buffer.push((char *) &q, sizeof (MESSAGE_TYPE));
    }

    //Join Worker Threads
    for(int i = 0; i<w; i++)
    {
        workers[i].join();
    }

    cout << " done joining" << endl;

    //Send Quit Messages for Histogram
    for(int i = 0; i<h; i++)
    {
        ResponseData q = {-1, 0};
        response.push((char *) &q, sizeof (MESSAGE_TYPE));
    }

    //Join Histogram Threads
    for(int i = 0; i<h; i++)
    {
        histograms[i].join();
    }


    gettimeofday (&end, 0);
    // print the results
	hc.print ();
    int secs = (end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)/(int) 1e6;
    int usecs = (int)(end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)%((int) 1e6);
    cout << "Took " << secs << " seconds and " << usecs << " micro seconds" << endl;

    MESSAGE_TYPE q = QUIT_MSG;
    chan->cwrite ((char *) &q, sizeof (MESSAGE_TYPE));
    cout << "All Done!!!" << endl;
    delete chan;
    
    wait(0);
}

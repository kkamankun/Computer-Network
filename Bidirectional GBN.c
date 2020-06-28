//////////////////////////////////////////////////////////
// File Name		: prog2.c			//
// Date			: 2020/05/22			//
// OS			: Windows 10			//
// Author		: Park Tae Sung			//
// ---------------------------------------------------- //
// Title: Computer Networking Assignment #2		//
// Description: Bidirectional GBN			//
//////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h> /* for malloc, free, srand, rand */

/*******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional or bidirectional
   data transfer protocols (from A to B. Bidirectional transfer of data
   is for extra credit and is not required).  Network properties:
   - one way network delay averages five time units (longer if there
	 are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
	 or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
	 (although some can be lost).
**********************************************************************/

#define BIDIRECTIONAL 1    /* change to 1 if you're doing extra credit */
/* and write a routine called B_output */

/* a "msg" is the data unit passed from layer 5 (teachers code) to layer  */
/* 4 (students' code).  It contains the data (characters) to be delivered */
/* to layer 5 via the students transport level protocol entities.         */
struct msg {
	char data[20];
};

/* a packet is the data unit passed from layer 4 (students code) to layer */
/* 3 (teachers code).  Note the pre-defined packet structure, which all   */
/* students must follow. */
struct pkt {
	int seqnum;
	int acknum;
	int checksum;
	char payload[20];
};

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

/* Here I define some function prototypes to avoid warnings */
/* in my compiler. Also I declared these functions void and */
/* changed the implementation to match */
void init();
void generate_next_arrival();
void insertevent(struct event *p);
void stoptimer(int);
void starttimer(int, float);
void tolayer3(int, struct pkt);
void tolayer5(int, char[20]);

#define WINDOWSIZE 8
#define MAXBUFSIZE 50

#define RTT 15.0

#define TRUE 1
#define FALSE 0

#define   A    0   
#define   B    1

int expectedseqnum_A; /* expected sequence number at A side */
int expectedseqnum_B; /* expected sequence number at B side */

int nextseqnum_A; /* next sequence number to use in A */
int nextseqnum_B; /* next sequence number to use in B */
int base_A; /* the head of A window */
int base_B; /* the head of A window */

int acknum_A; /* acknowledgement number to use in A */
int acknum_B; /* acknowledgement number to use in B */

struct pkt pktbuf_A[MAXBUFSIZE]; /* packets buffer at A side */
int msgnum_A; /* message number of A */
struct pkt pktbuf_B[MAXBUFSIZE]; /* packets buffer at B side */
int msgnum_B; /* message number of B */

/* compute checksum */
void ComputeChecksum(struct pkt *pktptr)
{
	int checksum;
	int i;

	checksum = pktptr->seqnum;
	checksum = checksum + pktptr->acknum;
	for (i = 0; i < 20; i++)
		checksum = checksum + (int)(pktptr->payload[i]);
	checksum = 0 - checksum;
	pktptr->checksum = checksum;
}

/* check corrupted packet*/
int CheckCorrupted(struct pkt packet)
{
	int checksum;
	int i;

	checksum = packet.seqnum;
	checksum = checksum + packet.acknum;
	for (i = 0; i < 20; i++)
		checksum = checksum + (int)(packet.payload[i]);
	if ((packet.checksum + checksum) == 0)
		return(FALSE);
	else
		return(TRUE);
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
	int i;
	struct pkt sendpkt;

	/* update state variables */
	acknum_A = expectedseqnum_A - 1;

	printf("----A: New message arrives, create packet and send ACK\n");
	/* create packet */
	sendpkt.seqnum = msgnum_A;
	sendpkt.acknum = acknum_A;
	for (i = 0; i < 20; i++)
		sendpkt.payload[i] = message.data[i];
	/* computer checksum */
	ComputeChecksum(&sendpkt);

	/* if window is not full */
	if (nextseqnum_A < base_A + WINDOWSIZE) {
		printf("----A: Send window is not full, send out packet to layer3!\n");

		/* copy the packet to packets buffer */
		pktbuf_A[nextseqnum_A] = sendpkt;

		/* send out packet */
		tolayer3(A, sendpkt);
		printf("seq: %d, ack: %d\n", sendpkt.seqnum, sendpkt.acknum);

		/* if it is the first packet in window, start timeout */
		if (base_A == nextseqnum_A) {
			starttimer(A, RTT);
			printf("----A: start a new timer!\n");
		}

		/* update state variables */
		nextseqnum_A = nextseqnum_A + 1;
	}
	/* if window is full */
	else {
		printf("----A: Send window is full,");
		/* buffer the packet */
		pktbuf_A[msgnum_A] = sendpkt;
	}
	/* update state variables */
	msgnum_A++;
}

/* called from layer 5, passed the data to be sent to other side */
void B_output(struct msg message)
{
	int i;
	struct pkt sendpkt;

	/* update state variables */
	acknum_B = expectedseqnum_B - 1;

	printf("----B: New message arrives, create packet and send ACK\n");
	/* create packet */
	sendpkt.seqnum = msgnum_B;
	sendpkt.acknum = acknum_B;
	for (i = 0; i < 20; i++)
		sendpkt.payload[i] = message.data[i];
	/* computer checksum */
	ComputeChecksum(&sendpkt);

	/* if window is not full */
	if (nextseqnum_B < base_B + WINDOWSIZE) {
		printf("----B: Send window is not full, send out packet to layer3!\n");

		/* copy the packet to packets buffer */
		pktbuf_B[nextseqnum_B] = sendpkt;

		/* send out packet */
		tolayer3(B, sendpkt);
		printf("seq: %d, ack: %d\n", sendpkt.seqnum, sendpkt.acknum);

		/* if it is the first packet in window, start timeout */
		if (base_B == nextseqnum_B) {
			starttimer(B, RTT);
			printf("----B: start a new timer!\n");
		}

		/* update state variables */
		nextseqnum_B = nextseqnum_B + 1;
	}
	/* if window is full */
	else {
		printf("----B: Send window is full,");
		/* buffer the packet */
		pktbuf_B[msgnum_B] = sendpkt;
	}
	/* update state variables */
	msgnum_B++;
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
	/* if not corrupted and received packet is in order */
	if ((CheckCorrupted(packet) == FALSE) && (packet.seqnum == expectedseqnum_A)) {
		printf("----A: packet %d is correctly received!\n", packet.seqnum);
		printf("seq: %d, ack: %d\n", packet.seqnum, packet.acknum);

		/* update state variables */
		base_A = packet.acknum + 1;
		if (base_A == nextseqnum_A)
			stoptimer(A);
		else
			starttimer(A, RTT);
		expectedseqnum_A = expectedseqnum_A + 1;

		/* deliver received packet to layer 5 */
		tolayer5(A, packet.payload);
	}
	/* otherwise, discard the packet */
	else
		printf("----A: packet %d, do nothing!\n", packet.seqnum);
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
	int i;

	printf("----A: time out, resend packets!\n");
	/* start timer */
	starttimer(A, RTT);
	/* resend all packets not acked */
	for (i = base_A; i <= nextseqnum_A - 1; i++)
		tolayer3(A, pktbuf_A[i]);
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
	expectedseqnum_A = 1;

	nextseqnum_A = 1;
	base_A = 1;

	acknum_A = 0;

	msgnum_A = 1;

}


/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
	/* if not corrupted and received packet is in order */
	if ((CheckCorrupted(packet) == FALSE) && (packet.seqnum == expectedseqnum_B)) {
		printf("----B: packet %d is correctly received!\n", packet.seqnum);
		printf("seq: %d, ack: %d\n", packet.seqnum, packet.acknum);

		/* update state variables */
		base_B = packet.acknum + 1;
		if (base_B == nextseqnum_B)
			stoptimer(B);
		else
			starttimer(B, RTT);
		expectedseqnum_B = expectedseqnum_B + 1;

		/* deliver received packet to layer 5 */
		tolayer5(B, packet.payload);
	}
	/* otherwise, discard the packet */
	else
		printf("----B: packet %d, do nothing!\n", packet.seqnum);
}

/* called when B's timer goes off */
void B_timerinterrupt()
{
	int i;

	printf("----B: time out, resend packets!\n");
	/* start timer */
	starttimer(B, RTT);
	/* resend all packets not acked */
	for (i = base_B; i <= nextseqnum_B - 1; i++)
		tolayer3(B, pktbuf_B[i]);
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
	expectedseqnum_B = 1;

	nextseqnum_B = 1;
	base_B = 1;

	acknum_B = 0;

	msgnum_B = 1;

}


/*****************************************************************
***************** NETWORK EMULATION CODE STARTS BELOW ***********
The code below emulates the layer 3 and below network environment:
  - emulates the tranmission and delivery (possibly with bit-level corruption
	and packet loss) of packets across the layer 3/4 interface
  - handles the starting/stopping of a timer, and generates timer
	interrupts (resulting in calling students timer handler).
  - generates message to be sent (passed from later 5 to 4)

THERE IS NOT REASON THAT ANY STUDENT SHOULD HAVE TO READ OR UNDERSTAND
THE CODE BELOW.  YOU SHOLD NOT TOUCH, OR REFERENCE (in your code) ANY
OF THE DATA STRUCTURES BELOW.  If you're interested in how I designed
the emulator, you're welcome to look at the code - but again, you should have
to, and you defeinitely should not have to modify
******************************************************************/

struct event {
	float evtime;           /* event time */
	int evtype;             /* event type code */
	int eventity;           /* entity where event occurs */
	struct pkt *pktptr;     /* ptr to packet (if any) assoc w/ this event */
	struct event *prev;
	struct event *next;
};
struct event *evlist = NULL;   /* the event list */

/* possible events: */
#define  TIMER_INTERRUPT 0
#define  FROM_LAYER5     1
#define  FROM_LAYER3     2

#define  OFF             0
#define  ON              1



int TRACE = 1;             /* for my debugging */
int nsim = 0;              /* number of messages from 5 to 4 so far */
int nsimmax = 0;           /* number of msgs to generate, then stop */
float time = (float)0.000;
float lossprob;            /* probability that a packet is dropped  */
float corruptprob;         /* probability that one bit is packet is flipped */
float lambda;              /* arrival rate of messages from layer 5 */
int   ntolayer3;           /* number sent into layer 3 */
int   nlost;               /* number lost in media */
int ncorrupt;              /* number corrupted by media*/

void main()
{
	struct event *eventptr;
	struct msg  msg2give;
	struct pkt  pkt2give;

	int i, j;
	/* char c; // Unreferenced local variable removed */

	init();
	A_init();
	B_init();

	while (1) {
		eventptr = evlist;            /* get next event to simulate */
		if (eventptr == NULL)
			goto terminate;
		evlist = evlist->next;        /* remove this event from event list */
		if (evlist != NULL)
			evlist->prev = NULL;
		if (TRACE >= 2) {
			printf("\nEVENT time: %f,", eventptr->evtime);
			printf("  type: %d", eventptr->evtype);
			if (eventptr->evtype == 0)
				printf(", timerinterrupt  ");
			else if (eventptr->evtype == 1)
				printf(", fromlayer5 ");
			else
				printf(", fromlayer3 ");
			printf(" entity: %d\n", eventptr->eventity);
		}
		time = eventptr->evtime;        /* update time to next event time */
		if (nsim == nsimmax)
			break;                        /* all done with simulation */
		if (eventptr->evtype == FROM_LAYER5) {
			generate_next_arrival();   /* set up future arrival */
			/* fill in msg to give with string of same letter */
			j = nsim % 26;
			for (i = 0; i < 20; i++)
				msg2give.data[i] = 97 + j;
			if (TRACE > 2) {
				printf("          MAINLOOP: data given to student: ");
				for (i = 0; i < 20; i++)
					printf("%c", msg2give.data[i]);
				printf("\n");
			}
			nsim++;
			if (eventptr->eventity == A)
				A_output(msg2give);
			else
				B_output(msg2give);
		}
		else if (eventptr->evtype == FROM_LAYER3) {
			pkt2give.seqnum = eventptr->pktptr->seqnum;
			pkt2give.acknum = eventptr->pktptr->acknum;
			pkt2give.checksum = eventptr->pktptr->checksum;
			for (i = 0; i < 20; i++)
				pkt2give.payload[i] = eventptr->pktptr->payload[i];
			if (eventptr->eventity == A)      /* deliver packet by calling */
				A_input(pkt2give);            /* appropriate entity */
			else
				B_input(pkt2give);
			free(eventptr->pktptr);          /* free the memory for packet */
		}
		else if (eventptr->evtype == TIMER_INTERRUPT) {
			if (eventptr->eventity == A)
				A_timerinterrupt();
			else
				B_timerinterrupt();
		}
		else {
			printf("INTERNAL PANIC: unknown event type \n");
		}
		free(eventptr);
	}

terminate:
	printf(" Simulator terminated at time %f\n after sending %d msgs from layer5\n", time, nsim);
}



void init()                         /* initialize the simulator */
{
	int i;
	float sum, avg;
	float jimsrand();


	printf("-----  Stop and Wait Network Simulator Version 1.1 -------- \n\n");
	printf("Enter the number of messages to simulate: ");
	scanf_s("%d", &nsimmax);
	printf("Enter  packet loss probability [enter 0.0 for no loss]:");
	scanf_s("%f", &lossprob);
	printf("Enter packet corruption probability [0.0 for no corruption]:");
	scanf_s("%f", &corruptprob);
	printf("Enter average time between messages from sender's layer5 [ >0.0]:");
	scanf_s("%f", &lambda);
	printf("Enter TRACE:");
	scanf_s("%d", &TRACE);

	srand(9999);              /* init random number generator */
	sum = (float)0.0;         /* test random number generator for students */
	for (i = 0; i < 1000; i++)
		sum = sum + jimsrand();    /* jimsrand() should be uniform in [0,1] */
	avg = sum / (float)1000.0;
	if (avg < 0.25 || avg > 0.75) {
		printf("It is likely that random number generation on your machine\n");
		printf("is different from what this emulator expects.  Please take\n");
		printf("a look at the routine jimsrand() in the emulator code. Sorry. \n");
		exit(0);
	}

	ntolayer3 = 0;
	nlost = 0;
	ncorrupt = 0;

	time = (float)0.0;                    /* initialize time to 0.0 */
	generate_next_arrival();     /* initialize event list */
}

/****************************************************************************/
/* jimsrand(): return a float in range [0,1].  The routine below is used to */
/* isolate all random number generation in one location.  We assume that the*/
/* system-supplied rand() function return an int in therange [0,mmm]        */
/****************************************************************************/
float jimsrand()
{
	double mmm = RAND_MAX;   /* largest int  - MACHINE DEPENDENT!!!!!!!!   */
	float x;                   /* individual students may need to change mmm */
	x = (float)(rand() / mmm);            /* x should be uniform in [0,1] */
	return(x);
}

/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/

void generate_next_arrival()
{
	double x, log(), ceil();
	struct event *evptr;
	/* char *malloc(); // malloc redefinition removed */
	/* float ttime; // Unreferenced local variable removed */
	/* int tempint; // Unreferenced local variable removed */

	if (TRACE > 2)
		printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");

	x = lambda * jimsrand() * 2;  /* x is uniform on [0,2*lambda] */
							  /* having mean of lambda        */
	evptr = (struct event *)malloc(sizeof(struct event));
	evptr->evtime = (float)(time + x);
	evptr->evtype = FROM_LAYER5;
	if (BIDIRECTIONAL && (jimsrand() > 0.5))
		evptr->eventity = B;
	else
		evptr->eventity = A;
	insertevent(evptr);
}


void insertevent(struct event *p)
{
	struct event *q, *qold;

	if (TRACE > 2) {
		printf("            INSERTEVENT: time is %lf\n", time);
		printf("            INSERTEVENT: future time will be %lf\n", p->evtime);
	}
	q = evlist;     /* q points to header of list in which p struct inserted */
	if (q == NULL) {   /* list is empty */
		evlist = p;
		p->next = NULL;
		p->prev = NULL;
	}
	else {
		for (qold = q; q != NULL && p->evtime > q->evtime; q = q->next)
			qold = q;
		if (q == NULL) {   /* end of list */
			qold->next = p;
			p->prev = qold;
			p->next = NULL;
		}
		else if (q == evlist) { /* front of list */
			p->next = evlist;
			p->prev = NULL;
			p->next->prev = p;
			evlist = p;
		}
		else {     /* middle of list */
			p->next = q;
			p->prev = q->prev;
			q->prev->next = p;
			q->prev = p;
		}
	}
}

void printevlist()
{
	struct event *q;
	/* int i; // Unreferenced local variable removed */
	printf("--------------\nEvent List Follows:\n");
	for (q = evlist; q != NULL; q = q->next) {
		printf("Event time: %f, type: %d entity:%d\n", q->evtime, q->evtype, q->eventity);
	}
	printf("--------------\n");
}



/********************** Student-callable ROUTINES ***********************/

/* called by students routine to cancel a previously-started timer */
void stoptimer(int AorB) /* A or B is trying to stop timer */
{
	struct event *q;/* ,*qold; // Unreferenced local variable removed */

	if (TRACE > 2)
		printf("          STOP TIMER: stopping timer at %f\n", time);
	/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
	for (q = evlist; q != NULL; q = q->next)
		if ((q->evtype == TIMER_INTERRUPT && q->eventity == AorB)) {
			/* remove this event */
			if (q->next == NULL && q->prev == NULL)
				evlist = NULL;         /* remove first and only event on list */
			else if (q->next == NULL) /* end of list - there is one in front */
				q->prev->next = NULL;
			else if (q == evlist) { /* front of list - there must be event after */
				q->next->prev = NULL;
				evlist = q->next;
			}
			else {     /* middle of list */
				q->next->prev = q->prev;
				q->prev->next = q->next;
			}
			free(q);
			return;
		}
	printf("Warning: unable to cancel your timer. It wasn't running.\n");
}


void starttimer(int AorB, float increment) /* A or B is trying to stop timer */
{

	struct event *q;
	struct event *evptr;
	/* char *malloc(); // malloc redefinition removed */

	if (TRACE > 2)
		printf("          START TIMER: starting timer at %f\n", time);
	/* be nice: check to see if timer is already started, if so, then  warn */
   /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
	for (q = evlist; q != NULL; q = q->next)
		if ((q->evtype == TIMER_INTERRUPT && q->eventity == AorB)) {
			printf("Warning: attempt to start a timer that is already started\n");
			return;
		}

	/* create future event for when timer goes off */
	evptr = (struct event *)malloc(sizeof(struct event));
	evptr->evtime = (float)(time + increment);
	evptr->evtype = TIMER_INTERRUPT;
	evptr->eventity = AorB;
	insertevent(evptr);
}


/************************** TOLAYER3 ***************/
void tolayer3(int AorB, struct pkt packet) /* A or B is trying to stop timer */
{
	struct pkt *mypktptr;
	struct event *evptr, *q;
	/* char *malloc(); // malloc redefinition removed */
	float lastime, x, jimsrand();
	int i;


	ntolayer3++;

	/* simulate losses: */
	if (jimsrand() < lossprob) {
		nlost++;
		if (TRACE > 0)
			printf("          TOLAYER3: packet being lost\n");
		return;
	}

	/* make a copy of the packet student just gave me since he/she may decide */
	/* to do something with the packet after we return back to him/her */
	mypktptr = (struct pkt *)malloc(sizeof(struct pkt));
	mypktptr->seqnum = packet.seqnum;
	mypktptr->acknum = packet.acknum;
	mypktptr->checksum = packet.checksum;
	for (i = 0; i < 20; i++)
		mypktptr->payload[i] = packet.payload[i];
	if (TRACE > 2) {
		printf("          TOLAYER3: seq: %d, ack %d, check: %d ", mypktptr->seqnum,
			mypktptr->acknum, mypktptr->checksum);
		for (i = 0; i < 20; i++)
			printf("%c", mypktptr->payload[i]);
		printf("\n");
	}

	/* create future event for arrival of packet at the other side */
	evptr = (struct event *)malloc(sizeof(struct event));
	evptr->evtype = FROM_LAYER3;   /* packet will pop out from layer3 */
	evptr->eventity = (AorB + 1) % 2; /* event occurs at other entity */
	evptr->pktptr = mypktptr;       /* save ptr to my copy of packet */
  /* finally, compute the arrival time of packet at the other end.
	 medium can not reorder, so make sure packet arrives between 1 and 10
	 time units after the latest arrival time of packets
	 currently in the medium on their way to the destination */
	lastime = time;
	/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next) */
	for (q = evlist; q != NULL; q = q->next)
		if ((q->evtype == FROM_LAYER3 && q->eventity == evptr->eventity))
			lastime = q->evtime;
	evptr->evtime = lastime + 1 + 9 * jimsrand();



	/* simulate corruption: */
	if (jimsrand() < corruptprob) {
		ncorrupt++;
		if ((x = jimsrand()) < .75)
			mypktptr->payload[0] = 'Z';   /* corrupt payload */
		else if (x < .875)
			mypktptr->seqnum = 999999;
		else
			mypktptr->acknum = 999999;
		if (TRACE > 0)
			printf("          TOLAYER3: packet being corrupted\n");
	}

	if (TRACE > 2)
		printf("          TOLAYER3: scheduling arrival on other side\n");
	insertevent(evptr);
}

void tolayer5(int AorB, char datasent[20])
{
	int i;
	if (TRACE > 2) {
		printf("          TOLAYER5: data received: ");
		for (i = 0; i < 20; i++)
			printf("%c", datasent[i]);
		printf("\n");
	}

}

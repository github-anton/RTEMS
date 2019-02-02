#include "ThreadedSpaceWireRw.h"
#include <stdint.h>

#pragma pack(push, 1)
typedef struct {
	u_char originAddr ;
	u_char packetNo ;
	u_char needReply ;			// boolean
	struct timespec originTime ;
	uint32_t crc32 ;
} th_packet_hdr_t;
#pragma pack(pop)

struct timespec sentPointTime[256] ;

/* CRC-32C (iSCSI) polynomial in reversed bit order. */
/* #define POLY 0x82f63b78 */
/* CRC-32 (Ethernet, ZIP, etc.) polynomial in reversed bit order. */
#define POLY 0xedb88320

uint32_t crc32c(uint32_t crc, const u_char *buf, size_t len)
{
    int k;

    crc = ~crc;
    while (len--) {
        crc ^= *buf++;
        for (k = 0; k < 8; k++)
            crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
    }
    return ~crc;
}

void *readerProc(void *arg) {
	u_char packet[TH_MAX_PACKET_LEN] ;
	size_t wrongPackets = 0 ;
	u_char packetNo[256] = {0} ;
	th_packet_hdr_t *pHdr ;

	pHdr = (th_packet_hdr_t*)&packet[0] ;

	// Setting up the transmission speed
	TRACE("Setting up speed to %iMbit/s\n", READER_REPLY_RATE) ;
	int stat = pReader->setTxSpeed(READER_REPLY_RATE) ;
	TRACE("setTxSpeed() returned %i\n", stat) ;

	TRACE("Start reading...\n") ;

	// Just print the received data...
	while(true) {
		bool connected = pReader->isConnected() ;
		if (connected) {
			DTRACE("CONNECTED\n") ;

            int nBytes = 0 ;
            nBytes = pReader->recv(packet, sizeof(packet)) ;
            DTRACE("<- Received a packet size=%i, from addr=%i, firts 32 bytes:\n", nBytes, pHdr->originAddr) ;
            if (nBytes > 0) {
                // Dump first 16 bytes
#ifdef DEBUG
                hexdump(packet, 32) ;
#endif
                u_char fromAddr = pHdr->originAddr ;

#ifdef CHECK_RECEIVED_PACKET
                if ( !checkPacket(packetNo[fromAddr], packet, sizeof(packet)) ) {	// WRONG
                    wrongPackets ++ ;
                    packetNo[fromAddr] = pHdr->packetNo + 1 ;
                } else {
                	packetNo[fromAddr] ++ ;
                }
#endif
#ifdef REPLY_ON_PACKET
                if(pHdr->needReply) {
                	pHdr->originAddr = readerAddr[0] ;
                	DTRACE("-> Send reply: toAddr=%i, packetNo=%i, size=%u\n", fromAddr, pHdr->packetNo, sizeof(th_packet_hdr_t)) ;
                	pReader->sendTo(&fromAddr, packet, sizeof(th_packet_hdr_t)) ;
                }
#endif
            }
		} else {
			DTRACE("DISCONNECTED\n") ;
#ifdef ENABLE_READER_DISCONNECTED_SNOOZE
            SNOOZE(TH_READER_DISCONNECTED_SNOOZE_TIME_MS) ;
#endif
		}
		DTRACE("wrongPackets=%u\n", wrongPackets) ;
	}
	return NULL ;
}

void *writerProc(void *arg) {
	u_char packet[TH_MAX_PACKET_LEN] ;
	size_t frameNo = 0;
	u_char packetNo = 0;
	u_int wrongPackets = 0 ;
	struct timespec tsend, trecv ;
	th_packet_hdr_t *pHdr ;
	u_char	*pData ;

	pHdr = (th_packet_hdr_t*)&packet[0] ;
	pData = &packet[sizeof(th_packet_hdr_t)] ;

	// Setting up the transmission speed
    TRACE("Setting up speed to %iMbit/s\n", WRITER_RATE) ;
    int stat = pWriter->setTxSpeed(WRITER_RATE) ;
    TRACE("setTxSpeed() returned %i\n", stat) ;

	TRACE("Start writing...\n") ;
	while (true) {
		DTRACE("========== Frame %u ===========\n", frameNo++) ;

		pHdr->originAddr = writerAddr[0] ;
#ifdef WRITER_NEED_REPLY
		pHdr->needReply = true ;
#else
		pHdr->needReply = false ;
#endif

#ifdef FILL_WRITER_PACKET
		// Fill the data buffer
		pHdr->packetNo = packetNo ;
		for (size_t i = 0; i < TH_MAX_PACKET_LEN-sizeof(th_packet_hdr_t); i++) {
			pData[i] = i + packetNo ;
		}
#endif
		// Calculate CRC32
		pHdr->crc32 = crc32c(0, pData, TH_MAX_PACKET_LEN-sizeof(th_packet_hdr_t)) ;

		bool connected = pWriter->isConnected() ;

		if ( connected ) {
			// If the connection is established print status and send
			// some data...

			DTRACE ("CONNECTED\n") ;

			DTRACE("-> Send a packet no=%i size=%u\n", packetNo, sizeof(packet)) ;
			//hexdump(data, sizeof(data)) ;
			clock_gettime(CLOCK_REALTIME, &tsend) ;	// begin point of time
			pHdr->originTime = tsend ;
			if ( pWriter->sendTo(readerAddr, packet, sizeof(packet)) == -1 ) {
				fprintf(stderr, "writerProc(): Sending ERROR, write() returned -1\n") ;
			}
			clock_gettime(CLOCK_REALTIME, &tsend) ;	// begin point of time
			sentPointTime[packetNo] = tsend ;
#if defined(WRITER_NEED_REPLY) && defined(RECEIVE_REPLY_SYNC)
			int nBytes = pWriter->recv(packet, sizeof(packet)) ;
			if (nBytes > 0) {
				clock_gettime(CLOCK_REALTIME, &trecv) ; // end point of time
				float delta = (trecv.tv_nsec - pHdr->originTime.tv_nsec) / 1000000.0 + (trecv.tv_sec - pHdr->originTime.tv_sec)*1000.0 ;
				TRACE("<- Received a reply size=%i, from addr=%i, sendRecvTime=%f ms\n", nBytes, pHdr->originAddr, delta) ;
				if ( !checkPacket(packetNo, packet, sizeof(packet)) ) {	// WRONG
				     wrongPackets ++ ;
				}
				DTRACE("wrongPackets=%u\n", wrongPackets) ;
			} else {
				DTRACE("<- Receiving a reply is FAIL\n") ;
			}
#endif	// REPLY_ON_PACKET && RECEIVE_REPLY_SYNC
			DTRACE("Sending time=%f ms\n", (sentPointTime[packetNo].tv_nsec - pHdr->originTime.tv_nsec)/1000000.0 +(sentPointTime[packetNo].tv_sec - pHdr->originTime.tv_sec)*1000.0) ;
		} else {
			// If disconnected print that.
			DTRACE ("DISCONNECTED\n") ;
		}

#ifdef ENABLE_WRITER_SNOOZE
		// Wait a while
		SNOOZE(TH_WRITER_SNOOZE_TIME_MS) ;
#endif

		packetNo ++ ;
	}
	return NULL ;
}

#if defined(REPLY_ON_PACKET) && defined(RECEIVE_REPLY_ASYNC)
void *replyCheckerProc(void *arg) {
	struct timespec trecv ;
	u_int wrongPackets = 0 ;
	u_char packet[TH_MAX_PACKET_LEN] ;
	th_packet_hdr_t	*pHdr ;
	u_char packetNo = 0 ;

	pHdr = (th_packet_hdr_t*)&packet[0] ;

	while(true) {
		int nBytes = pWriter->recv(packet, sizeof(packet)) ;
		if (nBytes > 0) {
			clock_gettime(CLOCK_REALTIME, &trecv) ; // end point of time
			float deltaOrigin = (trecv.tv_nsec - pHdr->originTime.tv_nsec) / 1000000.0 + (trecv.tv_sec - pHdr->originTime.tv_sec)*1000.0 ;
			float deltaSent = (trecv.tv_nsec - sentPointTime[pHdr->packetNo].tv_nsec) / 1000000.0 + (trecv.tv_sec - sentPointTime[pHdr->packetNo].tv_sec)*1000.0 ;
			DTRACE("<- Received a reply size=%i, from addr=%i\n", nBytes, pHdr->originAddr) ;
            DTRACE("send-reply-time=%f ms, reply-time=%f ms\n", deltaOrigin, deltaSent) ;
			if ( !checkPacket(packetNo, packet, sizeof(packet)) ) {	// WRONG
				wrongPackets ++ ;
			}
			DTRACE("wrongPackets=%u\n", wrongPackets) ;
		}
		packetNo++ ;
	}

	return NULL ;
}
#endif

void hexdump ( u_char buf[], size_t len ) {
	for ( size_t i = 0; i < len; i++ ) {
		printf("0x%X ", buf[i]) ;
	}
	printf("\n") ;
}

bool checkPacket(u_char packetNo, u_char buf[], size_t len) {
	th_packet_hdr_t *pHdr = (th_packet_hdr_t*)buf ;
	u_char *pData = &buf[sizeof(th_packet_hdr_t)] ;
	if ( pHdr->packetNo != packetNo ) {
		DTRACE("Wrong packetNo, no=%i, expectedNo=%i, originAddr=%i\n", pHdr->packetNo, packetNo, pHdr->originAddr) ;
		return false ;
	} else {
		DTRACE("The packetNo is OK, no=%i, expectedNo=%i, originAddr=%i\n", pHdr->packetNo, packetNo, pHdr->originAddr) ;
	}

	/* Check CRC32 */
	u_int crc32 = crc32c(0, pData, len - sizeof(th_packet_hdr_t)) ;
	if ( pHdr->crc32 != crc32 ) {
		DTRACE("CRC32 missmatch: should be 0x%x, but it's 0x%x\n", pHdr->crc32, crc32) ;
		return false ;
	} else {
		DTRACE("CRC32 is OK\n") ;
	}

	return true ;
}

void *timeCodeReceiverProc(void *arg) {
    while(true) {
        u_char time = pReader->waitTimeCode() ;
        DTRACE("Received TimeCode=%i\n", time) ;
    }
}

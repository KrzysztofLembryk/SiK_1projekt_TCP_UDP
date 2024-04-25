#ifndef PROTCONST_H
#define PROTCONST_H

// MAX_WAIT constant defines how many seconds server waits for sent package
#define MAX_WAIT 4

// If after MAX_WAIT seconds confirmation about sent package hasn't come from
//  server we send this package again
#define MAX_RETRANSMITS 1

#endif
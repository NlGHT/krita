#ifndef KTYPES_H_
#define KTYPES_H_

#include "kis_pointer_utils.h"


template<class T>
class KisWeakSharedPtr;
template<class T>
class KisSharedPtr;

template<class T> class QSharedPointer;
template<class T> class QWeakPointer;

class KisNode;
typedef KisSharedPtr<KisNode> KisNodeSP;
typedef KisWeakSharedPtr<KisNode> KisNodeWSP;
typedef QVector<KisNodeSP> vKisNodeSP;
typedef vKisNodeSP::iterator vKisNodeSP_it;
typedef vKisNodeSP::const_iterator vKisNodeSP_cit;

typedef QList<KisNodeSP> KisNodeList;
typedef QSharedPointer<KisNodeList> KisNodeListSP;

#endif // KTYPES_H_

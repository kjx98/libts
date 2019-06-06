#pragma once

#ifndef	__TS3_PITCH_HPP__
#define	__TS3_PITCH_HPP__

#include "ts3/types.h"
#include "ts3/timestamp.hpp"
#include "ts3/serialization.hpp"

namespace ts3 {
namespace pitch {
/*
 * Message types:
 */
enum msgType {
	MSG_SYSTEM_EVENT			= 'S',	/* Section 4.1. */
	MSG_SYMBOL_DIRECTORY		= 'R',	/* Section 4.2.1. */
	MSG_SYMBOL_TRADING_ACTION	= 'H',	/* Section 4.2.2. */
	MSG_ADD_ORDER				= 'A',	/* Section 4.3.1. */
	MSG_ORDER_EXECUTED			= 'E',	/* Section 4.4.1. */
	MSG_ORDER_EXECUTED_WITH_PRICE	= 'C',	/* Section 4.4.2. */
	MSG_ORDER_CANCEL			= 'X',	/* Section 4.4.3. */
	MSG_ORDER_DELETE			= 'D',	/* Section 4.4.4. */
	MSG_ORDER_REPLACE			= 'U',	/* Section 4.4.5. */
	MSG_TRADE					= 'P',	/* Section 4.5.1. */
	MSG_CROSS_TRADE				= 'Q',	/* Section 4.5.2. */
	MSG_BROKEN_TRADE			= 'B',	/* Section 4.5.3. */
	MSG_NOII					= 'I',	/* Section 4.6. */
};

/*
 * System event codes:
 */
enum eventCode {
	EVENT_START_OF_MESSAGES		= 'O',
	EVENT_START_OF_SYSTEM_HOURS	= 'S',
	EVENT_START_OF_MARKET_HOURS	= 'Q',
	EVENT_END_OF_MARKET_HOURS	= 'M',
	EVENT_END_OF_SYSTEM_HOURS	= 'E',
	EVENT_END_OF_MESSAGES		= 'C',
	EVENT_EMERGENCY_HALT		= 'A',
	EVENT_EMERGENCY_QUOTE_ONLY	= 'R',
	EVENT_EMERGENCY_RESUMPTION	= 'B',
};

/*
 * A data structure for PITCH messages.
 */
class pitchMessage {
public:
	pitchMessage() = default;
	pitchMessage(const u8 msgT, const uint16_t symIndex, const uint16_t trkNo,
				const int64_t timeUs) :
			msgType_(msgT), symbolIndex_(symIndex),
			trackingNo_(trkNo), timestamp_(timeUs%HourUs) {}
	virtual ~pitchMessage() = default;
	msgType	MessageType() { return (msgType)msgType_; }
	virtual int	marshal(void *bufP, int bLen) = 0;
	virtual	bool unmarshal(Serialization &sr) = 0;
	virtual	bool unmarshal(void *bufP, int bLen) = 0;
	uint16_t	SymbolIndex() { return symbolIndex_; }
	uint16_t	TrackingNo() { return trackingNo_; }
	uint32_t	TimeStamp() { return timestamp_; }
protected:
	u8		msgType_;		// enum msgType
	le16	symbolIndex_;	// symbol index
	le16	trackingNo_;
	le32	timestamp_;	// Hour based microseconds or nanoseconds
	// following actual message payload
};

/* MSG_SYSTEM_EVENT */
class pitchSystemEvent : public pitchMessage {
public:
	pitchSystemEvent() = default;
	pitchSystemEvent(const eventCode evtCode, const uint16_t symIndex,
		const uint16_t trackNo, const int64_t timeUs):
		pitchMessage(MSG_SYSTEM_EVENT, symIndex, trackNo, timeUs),
		eventCode_(evtCode), timeHours_(timeUs/HourUs) {}
	friend bool operator==(const pitchSystemEvent &lhs, const pitchSystemEvent& rhs) {
		if (lhs.eventCode_ != rhs.eventCode_ || lhs.symbolIndex_ != rhs.symbolIndex_
			|| lhs.trackingNo_ != rhs.trackingNo_) return false;
		return (lhs.timeHours_ == rhs.timeHours_ && lhs.timestamp_ == rhs.timestamp_);
	}
	int marshal(void *bufP, int bLen) {
		Serialization	sr(bufP, bLen);
		if (!sr.encode(msgType_, eventCode_, symbolIndex_, trackingNo_,
			timeHours_, timestamp_)) return -1;
		return sr.Size();
	}
	bool unmarshal(Serialization &sr) {
		if (!sr.decode(eventCode_, symbolIndex_, trackingNo_, timeHours_,
			timestamp_)) return false;
		msgType_ = MSG_SYSTEM_EVENT;
		return true;
	}
	bool unmarshal(void *bufP, int bLen) {
		Serialization	sr(bufP, bLen);
		return unmarshal(sr);
	}
	u8	EventCode() { return eventCode_; }
	uint32_t	TimeHours() { return timeHours_; }
protected:
	u8		eventCode_;	/* PITCH_EVENT_<code> */
	le32	timeHours_;	// hours after 1970/1/1 00:00 UTC
};

/* MSG_SYMBOL_DIRECTORY */
class pitchSymbolDirectory  : public pitchMessage {
public:
	pitchSymbolDirectory() = default;
	pitchSymbolDirectory(const u8 mktC, const char *contr, const u8 classi,
		const u8 prec, const uint16_t symIndex, const uint16_t trkNo,
		const int64_t timeUs, const uint32_t lotSize, const uint32_t turnMu,
		const uint32_t lowerL, const uint32_t upperL):
			pitchMessage(MSG_SYMBOL_DIRECTORY, symIndex, trkNo, timeUs),
			marketCategory_(mktC),
			classification_(classi), precision_(prec),
			roundLotSize_(lotSize), turnoverMulti_(turnMu),
			lowerLimit_(lowerL), upperLimit_(upperL) {
		memset(contract_, 0, sizeof(contract_));
		strncpy(contract_, contr, sizeof(contract_)-1);
	}
	friend bool operator==(const pitchSymbolDirectory &lhs, const pitchSymbolDirectory& rhs) {
		if (lhs.marketCategory_ != rhs.marketCategory_ ||
			lhs.classification_ != rhs.classification_ ||
			lhs.precision_ != rhs.precision_ ||
			lhs.symbolIndex_ != rhs.symbolIndex_ ||
			lhs.trackingNo_ != rhs.trackingNo_ ||
			lhs.turnoverMulti_ != rhs.turnoverMulti_ ||
			lhs.lowerLimit_ != rhs.lowerLimit_ ||
			lhs.upperLimit_ != rhs.upperLimit_) return false;
		return memcmp(lhs.contract_, rhs.contract_, sizeof(lhs.contract_)) == 0;
	}
	int marshal(void *bufP, int bLen) {
		Serialization	sr(bufP, bLen);
		if (!sr.encode(msgType_, marketCategory_)) return -1;
		if (!sr.encodeBytes((const u8*)contract_, sizeof(contract_))) return -1;
		if (!sr.encode(classification_, precision_, symbolIndex_, trackingNo_,
			timestamp_, roundLotSize_, turnoverMulti_, lowerLimit_,
			upperLimit_)) return -1;
		return sr.Size();
	}
	bool unmarshal(Serialization &sr) {
		if (!sr.decode1b(marketCategory_)) return false;
		if (!sr.decodeBytes((u8 *)contract_, sizeof(contract_))) return false;
		if (!sr.decode(classification_, precision_, symbolIndex_, trackingNo_,
            timestamp_, roundLotSize_, turnoverMulti_, lowerLimit_,
            upperLimit_)) return false;
		msgType_ = MSG_SYMBOL_DIRECTORY;
		return true;
	}
	bool unmarshal(void *bufP, int bLen) {
		Serialization	sr(bufP, bLen);
		return unmarshal(sr);
	}
protected:
	u8		marketCategory_;
	char	contract_[16];
	u8		classification_;
	u8		precision_;	// digits after dot
	le32	roundLotSize_;
	le32	turnoverMulti_;
	le32	lowerLimit_;
	le32	upperLimit_;
};

enum tradingState {
	TRADE_HALT	= 'H',
	TRADE_PREAUCTION = 'P',
	TRADE_AUCTION	= 'A',
	TRADE_PAUSE	= 'U',
	TRADE_NORMAL	= 'C',
	TRADE_BREAK	= 'B',
};

/* MSG_SYMBOL_TRADING_ACTION */
class symbolTradingAction  : public pitchMessage {
public:
	symbolTradingAction()=default;
	symbolTradingAction(const tradingState state, const uint16_t reas,
		const uint16_t symIndex, const uint16_t trkNo, const int64_t timeUs) :
		pitchMessage(MSG_SYMBOL_TRADING_ACTION, symIndex, trkNo, timeUs),
		tradingState_(state), reason_(reas) {}
	friend bool operator==(const symbolTradingAction &lhs, const symbolTradingAction& rhs) {
		if (lhs.tradingState_ != rhs.tradingState_ || lhs.reason_ != rhs.reason_
			|| lhs.symbolIndex_ != rhs.symbolIndex_ 
			|| lhs.trackingNo_ != rhs.trackingNo_) return false;
		return lhs.timestamp_ == rhs.timestamp_;
	}
	int marshal(void *bufP, int bLen) {
		Serialization	sr(bufP, bLen);
		if (!sr.encode(msgType_, tradingState_, reason_, symbolIndex_,
			trackingNo_, timestamp_)) return -1;
		return sr.Size();
	}
	bool unmarshal(Serialization &sr) {
		if (!sr.decode(tradingState_, reason_, symbolIndex_,
			trackingNo_, timestamp_)) return false;
		msgType_ = MSG_SYMBOL_TRADING_ACTION;
		return true;
	}
	bool unmarshal(void *bufP, int bLen) {
		Serialization	sr(bufP, bLen);
		return unmarshal(sr);
	}
protected:
	u8		tradingState_;
	le16	reason_;
};

enum buySellIndicator {
	PITCH_BUY_OPEN			= 'B',
	PITCH_SELL_OPEN			= 'S',
	PITCH_BUY_COVER			= 'C',
	PITCH_SELL_CLOSE		= 'O',
};

/* MSG_ADD_ORDER */
class addOrder  : public pitchMessage {
public:
	addOrder() = default;
	addOrder(const buySellIndicator bs, const uint16_t symIndex,
		const uint16_t trkNo, const int64_t timeUs, const uint64_t ordRef,
		const int32_t qty, const int32_t prc):
		pitchMessage(MSG_ADD_ORDER, symIndex, trkNo, timeUs),
		buySell_(bs), orderRefNo_(ordRef), qty_(qty), price_(prc) {}
	friend bool operator==(const addOrder &lhs, const addOrder& rhs) {
		if (lhs.buySell_ != rhs.buySell_ || lhs.symbolIndex_ != rhs.symbolIndex_
			|| lhs.trackingNo_ != rhs.trackingNo_ || lhs.timestamp_ != rhs.timestamp_
			|| lhs.orderRefNo_ != rhs.orderRefNo_ ) return false;
		return  lhs.qty_ == rhs.qty_ && lhs.price_ == rhs.price_;
	}
	int marshal(void *bufP, int bLen) {
		Serialization	sr(bufP, bLen);
		if (!sr.encode(msgType_, buySell_, symbolIndex_, trackingNo_, timestamp_,
			orderRefNo_, qty_, price_)) return -1;
		return sr.Size();
	}
	bool unmarshal(Serialization &sr) {
		if (!sr.decode(buySell_, symbolIndex_, trackingNo_, timestamp_,
			orderRefNo_, qty_, price_)) return false;
		msgType_ = MSG_ADD_ORDER;
		return true;
	}
	bool unmarshal(void *bufP, int bLen) {
		Serialization	sr(bufP, bLen);
		return unmarshal(sr);
	}
protected:
	u8		buySell_;
	le64	orderRefNo_;
	le32	qty_;
	le32	price_;
};

/* MSG_ORDER_EXECUTED */
class orderExecuted  : public pitchMessage {
public:
	orderExecuted() = default;
	orderExecuted(const char printA, const uint16_t symIndex,
		const uint16_t trkNo, const int64_t timeUs, const uint64_t ordRef,
		const int32_t qty, const uint64_t matchNo) :
		pitchMessage(MSG_ORDER_EXECUTED, symIndex, trkNo, timeUs),
		printable_(printA),  orderRefNo_(ordRef),
		qty_(qty), matchNo_(matchNo) {}
	friend bool operator==(const orderExecuted &lhs, const orderExecuted& rhs) {
		if (lhs.printable_ != rhs.printable_ || lhs.symbolIndex_ != rhs.symbolIndex_
			|| lhs.trackingNo_ != rhs.trackingNo_ || lhs.timestamp_ != rhs.timestamp_
			|| lhs.orderRefNo_ != rhs.orderRefNo_ || lhs.qty_ != rhs.qty_)
			return false;
		return lhs.matchNo_ == rhs.matchNo_;
	}
	int marshal(void *bufP, int bLen) {
		Serialization	sr(bufP, bLen);
		if (!sr.encode(msgType_, printable_, symbolIndex_, trackingNo_, timestamp_,
			orderRefNo_, qty_, matchNo_)) return -1;
		return sr.Size();
	}
	bool unmarshal(Serialization &sr) {
		if (!sr.decode( printable_, symbolIndex_, trackingNo_, timestamp_,
			orderRefNo_, qty_, matchNo_)) return false;
		msgType_ = MSG_ORDER_EXECUTED;
		return true;
	}
	bool unmarshal(void *bufP, int bLen) {
		Serialization	sr(bufP, bLen);
		return unmarshal(sr);
	}
protected:
	u8		printable_;
	le64	orderRefNo_;
	le32	qty_;
	le64	matchNo_;
};

/* MSG_ORDER_EXECUTED_WITH_PRICE */
class orderEexecutedWithPrice  : public pitchMessage {
public:
	orderEexecutedWithPrice() = default;
	orderEexecutedWithPrice(const char printA, const uint16_t symIndex,
		const uint16_t trkNo, const int64_t timeUs, const uint64_t ordRef,
		const int32_t qty, const uint64_t matchNo, const int32_t prc) :
		pitchMessage(MSG_ORDER_EXECUTED_WITH_PRICE, symIndex, trkNo, timeUs),
		printable_(printA), 
		orderRefNo_(ordRef), qty_(qty), matchNo_(matchNo), price_(prc) {}
	friend bool operator==(const orderEexecutedWithPrice &lhs, const orderEexecutedWithPrice& rhs)
	{
		if (lhs.printable_ != rhs.printable_ || lhs.symbolIndex_ != rhs.symbolIndex_
			|| lhs.trackingNo_ != rhs.trackingNo_ || lhs.timestamp_ != rhs.timestamp_
			|| lhs.orderRefNo_ != rhs.orderRefNo_ || lhs.qty_ != rhs.qty_)
			return false;
		return lhs.matchNo_ == rhs.matchNo_ && lhs.price_ == rhs.price_;
	}
	int marshal(void *bufP, int bLen) {
		Serialization	sr(bufP, bLen);
		if (!sr.encode(msgType_, printable_, symbolIndex_, trackingNo_, timestamp_,
			orderRefNo_, qty_, matchNo_, price_)) return -1;
		return sr.Size();
	}
	bool unmarshal(Serialization &sr) {
		if (!sr.decode( printable_, symbolIndex_, trackingNo_, timestamp_,
			orderRefNo_, qty_, matchNo_, price_)) return false;
		msgType_ = MSG_ORDER_EXECUTED_WITH_PRICE;
		return true;
	}
	bool unmarshal(void *bufP, int bLen) {
		Serialization	sr(bufP, bLen);
		return unmarshal(sr);
	}
protected:
	u8		printable_;
	le64	orderRefNo_;
	le32	qty_;		// executed Qty
	le64	matchNo_;
	le32	price_;		// executed price
};

enum cancelCode {
	PITCH_CANCEL_BYUSER =	'U',
	PITCH_CANCEL_ARB	=	'A',
	PITCH_CANCEL_ODDLOT	=	'O',	// not normalization lots
	PITCH_CANCEL_OOB	=	'B',	// out of price band
	PITCH_CANCEL_BREAK	=	'S',	// broken session
	PITCH_CANCEL_NORMAL	=	'N',	// order cancel out of normal trading
};

/* MSG_ORDER_CANCEL */
class orderCancel  : public pitchMessage {
public:
	orderCancel() = default;
	orderCancel(const cancelCode cCode, const uint16_t symIndex,
		const uint16_t trkNo, const int64_t timeUs,
		const uint64_t ordRef, const int32_t qty):
		pitchMessage(MSG_ORDER_CANCEL, symIndex, trkNo, timeUs),
		cancelReason_(cCode), orderRefNo_(ordRef), qty_(qty) {}
	friend bool operator==(const orderCancel &lhs, const orderCancel& rhs)
	{
		if (lhs.cancelReason_ != rhs.cancelReason_
			|| lhs.symbolIndex_ != rhs.symbolIndex_
			|| lhs.trackingNo_ != rhs.trackingNo_
			|| lhs.timestamp_ != rhs.timestamp_
			|| lhs.orderRefNo_ != rhs.orderRefNo_ ) return false;
		return lhs.qty_ == rhs.qty_;
	}
	int marshal(void *bufP, int bLen) {
		Serialization	sr(bufP, bLen);
		if (!sr.encode(msgType_, cancelReason_, symbolIndex_, trackingNo_,
			timestamp_, orderRefNo_, qty_)) return -1;
		return sr.Size();
	}
	bool unmarshal(Serialization &sr) {
		if (!sr.decode( cancelReason_, symbolIndex_, trackingNo_, timestamp_,
			orderRefNo_, qty_)) return false;
		msgType_ = MSG_ORDER_CANCEL;
		return true;
	}
	bool unmarshal(void *bufP, int bLen) {
		Serialization	sr(bufP, bLen);
		return unmarshal(sr);
	}
protected:
	u8		cancelReason_;
	le64	orderRefNo_;
	le32	qty_;
};

/* MSG_ORDER_DELETE */
class orderDelete  : public pitchMessage {
public:
	orderDelete() = default;
	orderDelete(const cancelCode cCode, const uint16_t symIndex,
		const uint16_t trkNo, const int64_t timeUs, const uint64_t ordRef):
		pitchMessage(MSG_ORDER_DELETE, symIndex, trkNo, timeUs),
		cancelReason_(cCode), orderRefNo_(ordRef){}
	friend bool operator==(const orderDelete &lhs, const orderDelete& rhs)
	{
		if (lhs.cancelReason_ != rhs.cancelReason_
			|| lhs.symbolIndex_ != rhs.symbolIndex_
			|| lhs.trackingNo_ != rhs.trackingNo_
			|| lhs.timestamp_ != rhs.timestamp_) return false;
		return lhs.orderRefNo_ == rhs.orderRefNo_;;
	}
	int marshal(void *bufP, int bLen) {
		Serialization	sr(bufP, bLen);
		if (!sr.encode(msgType_, cancelReason_, symbolIndex_, trackingNo_,
			timestamp_, orderRefNo_)) return -1;
		return sr.Size();
	}
	bool unmarshal(Serialization &sr) {
		if (!sr.decode( cancelReason_, symbolIndex_, trackingNo_, timestamp_,
			orderRefNo_)) return false;
		msgType_ = MSG_ORDER_DELETE;
		return true;
	}
	bool unmarshal(void *bufP, int bLen) {
		Serialization	sr(bufP, bLen);
		return unmarshal(sr);
	}
protected:
	u8		cancelReason_;
	le64	orderRefNo_;
};

/* MSG_ORDER_REPLACE */
class orderReplace  : public pitchMessage {
public:
	orderReplace() = default;
	orderReplace(const uint16_t symIndex, const uint16_t trkNo,
		const int64_t timeUs, const uint64_t ordRef, const uint64_t NewOrdRef,
		const int32_t qty, const int32_t prc):
		pitchMessage(MSG_ORDER_REPLACE, symIndex, trkNo, timeUs),
		orderRefNo_(ordRef), newOrderRefNo_(NewOrdRef), qty_(qty), price_(prc) {}
	friend bool operator==(const orderReplace &lhs, const orderReplace& rhs) {
		if (lhs.symbolIndex_ != rhs.symbolIndex_
			|| lhs.trackingNo_ != rhs.trackingNo_
			|| lhs.timestamp_ != rhs.timestamp_
			|| lhs.orderRefNo_ != rhs.orderRefNo_
			|| lhs.newOrderRefNo_ != rhs.newOrderRefNo_
			|| lhs.qty_ != rhs.qty_) return false;
		return lhs.price_ == rhs.price_;
	}
	int marshal(void *bufP, int bLen) {
		Serialization	sr(bufP, bLen);
		if (!sr.encode(msgType_, symbolIndex_, trackingNo_, timestamp_,
			orderRefNo_, newOrderRefNo_, qty_, price_)) return -1;
		return sr.Size();
	}
	bool unmarshal(Serialization &sr) {
		if (!sr.decode( symbolIndex_, trackingNo_, timestamp_, orderRefNo_,
			newOrderRefNo_, qty_, price_)) return false;
		msgType_ = MSG_ORDER_REPLACE;
		return true;
	}
	bool unmarshal(void *bufP, int bLen) {
		Serialization	sr(bufP, bLen);
		return unmarshal(sr);
	}
protected:
	le64	orderRefNo_;
	le64	newOrderRefNo_;
	le32	qty_;
	le32	price_;
};

/* MSG_TRADE */
class msgTrade  : public pitchMessage {
public:
	msgTrade() = default;
	msgTrade(const buySellIndicator bs, const uint16_t symIndex,
		const uint16_t trkNo, const int64_t timeUs, const uint64_t ordRef,
		const int32_t qty, const int32_t prc, const uint64_t mNo):
		pitchMessage(MSG_TRADE, symIndex, trkNo, timeUs),
		buySell_(bs), orderRefNo_(ordRef), qty_(qty), price_(prc),
		matchNo_(mNo) {}
	friend bool operator==(const msgTrade &lhs, const msgTrade& rhs) {
		if (lhs.buySell_ != rhs.buySell_ || lhs.symbolIndex_ != rhs.symbolIndex_
			|| lhs.trackingNo_ != rhs.trackingNo_
			|| lhs.timestamp_ != rhs.timestamp_
			|| lhs.orderRefNo_ != rhs.orderRefNo_
			|| lhs.qty_ != rhs.qty_) return false;
		return lhs.price_ == rhs.price_ && lhs.matchNo_ == rhs.matchNo_;
	}
	int marshal(void *bufP, int bLen) {
		Serialization	sr(bufP, bLen);
		if (!sr.encode(msgType_, buySell_, symbolIndex_, trackingNo_,
			timestamp_, orderRefNo_, qty_, price_, matchNo_)) return -1;
		return sr.Size();
	}
	bool unmarshal(Serialization &sr) {
		if (!sr.decode( buySell_, symbolIndex_, trackingNo_, timestamp_,
			orderRefNo_, qty_, price_, matchNo_)) return false;
		msgType_ = MSG_TRADE;
		return true;
	}
	bool unmarshal(void *bufP, int bLen) {
		Serialization	sr(bufP, bLen);
		return unmarshal(sr);
	}
protected:
	u8		buySell_;
	le64	orderRefNo_;
	le32	qty_;
	le32	price_;
	le64	matchNo_;
};

/* MSG_CROSS_TRADE */
class crossTrade  : public pitchMessage {
public:
	crossTrade() = default;
	crossTrade(const u8 ct, const uint16_t symIndex, const uint16_t trkNo,
		const int64_t timeUs, const int32_t qty, const int32_t prc,
		const int32_t op, const uint64_t mNo):
		pitchMessage(MSG_CROSS_TRADE, symIndex, trkNo, timeUs),
		crossType_(ct), qty_(qty), price_(prc),
		openInterest_(op), matchNo_(mNo) {}
	friend bool operator==(const crossTrade &lhs, const crossTrade& rhs) {
		if (lhs.crossType_ != rhs.crossType_
			|| lhs.symbolIndex_ != rhs.symbolIndex_
			|| lhs.trackingNo_ != rhs.trackingNo_
			|| lhs.timestamp_ != rhs.timestamp_
			|| lhs.qty_ != rhs.qty_ || lhs.price_ != rhs.price_
			|| lhs.openInterest_ != rhs.openInterest_) return false;
		return  lhs.matchNo_ == rhs.matchNo_;
	}
	int marshal(void *bufP, int bLen) {
		Serialization	sr(bufP, bLen);
		if (!sr.encode(msgType_, crossType_, symbolIndex_, trackingNo_,
			timestamp_, qty_, price_, openInterest_, matchNo_)) return -1;
		return sr.Size();
	}
	bool unmarshal(Serialization &sr) {
		if (!sr.decode(crossType_, symbolIndex_, trackingNo_, timestamp_,
			qty_, price_, openInterest_, matchNo_)) return false;
		msgType_ = MSG_CROSS_TRADE;
		return true;
	}
	bool unmarshal(void *bufP, int bLen) {
		Serialization	sr(bufP, bLen);
		return unmarshal(sr);
	}
protected:
	u8		crossType_;
	le32	qty_;
	le32	price_;
	le32	openInterest_;
	le64	matchNo_;
};

/* MSG_BROKEN_TRADE */
class brokenTrade  : public pitchMessage {
protected:
	char	BrokenTag;
	le64	MatchNumber;
};

/* MSG_NOII */
class msgNoii  : public pitchMessage {
protected:
	char	ImbalanceDirection;
	le64	PairedQty;
	le64	ImbalanceQty;
	le32	FarPrice;
	le32	NearPrice;
	le32	CurrentReferencePrice;
	char	CrossType;
	char	PriceVariationIndicator;
};

static inline	std::shared_ptr<pitchMessage> unmarshal(void *bufP, int bLen) {
	Serialization	sr(bufP, bLen);
	std::shared_ptr<pitchMessage>	ret;
	u8		msgT;
	if (!sr.decode(msgT)) return ret;
	std::shared_ptr<pitchMessage>	res;
	switch((msgType)msgT) {
	case MSG_SYSTEM_EVENT:
		res = std::make_shared<pitchSystemEvent>();
		break;
	case MSG_SYMBOL_DIRECTORY:
		res = std::make_shared<pitchSymbolDirectory>();
		break;
	case MSG_SYMBOL_TRADING_ACTION:
		res = std::make_shared<symbolTradingAction>();
		break;
	case MSG_ADD_ORDER:
		res = std::make_shared<addOrder>();
		break;
	case MSG_ORDER_EXECUTED:
		res = std::make_shared<orderExecuted>();
		break;
	case MSG_ORDER_EXECUTED_WITH_PRICE:
		res = std::make_shared<orderEexecutedWithPrice>();
		break;
	case MSG_ORDER_CANCEL:
		res = std::make_shared<orderCancel>();
		break;
	case MSG_ORDER_DELETE:
		res = std::make_shared<orderDelete>();
		break;
	case MSG_ORDER_REPLACE:
		res = std::make_shared<orderReplace>();
		break;
	case MSG_TRADE:
		res = std::make_shared<msgTrade>();
		break;
	case MSG_CROSS_TRADE:
		res = std::make_shared<crossTrade>();
		break;
	case MSG_NOII:
	case MSG_BROKEN_TRADE:
		return ret;
	}
	if (!res->unmarshal(sr)) {
		return ret;
	}
	return res;
}

} }
#endif	// __TS3_PITCH_HPP__

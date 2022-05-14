#pragma once

#ifndef	__TS3_PITCH_HPP__
#define	__TS3_PITCH_HPP__

#include "ts3/types.h"
#include "ts3/timestamp.hpp"
#include "ts3/serialization.hpp"

namespace ts3::pitch {

using namespace std;
constexpr int	pMsgSize=64;
/*
 * Message types:
 */
enum msgType : u8 {
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
enum eventCode : u8 {
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
	pitchMessage(const u8 msgT, const uint16_t symIndex, const uint16_t trkN,
				const int64_t timeUs) :
			msgType_(msgT), symbolIndex_(symIndex),
			trackingNo_(trkN), timestamp_(timeUs%HourUs) {}
	virtual ~pitchMessage() = default;
	msgType	MessageType() { return (msgType)msgType_; }
	virtual int	marshal(void *bufP, int bLen) = 0;
#ifdef	_NTEST
	virtual	bool unmarshal(Serialization &sr) = 0;
#endif
	uint16_t	SymbolIndex() const { return symbolIndex_; }
	uint16_t	TrackingNo() const { return trackingNo_; }
	uint32_t	TimeStamp() const { return timestamp_; }
	uint64_t	TimeStampUs(const time_t sec) const {
		time_t	res=timestamp_ / ts3::duration::us;
		int		us=timestamp_ % ts3::duration::us;
		// Adjust to near hour
		time_t	rem = (sec + 120) % 3600;
		res += (sec+120) - rem;
		uint64_t ret = (uint64_t)res * ts3::duration::us;
		ret += us;
		return ret;
	}
protected:
	u8		msgType_;		// enum msgType
	le16	symbolIndex_;	// symbol index
	le16	trackingNo_;
	le32	timestamp_;	// Hour based microseconds, or nanoseconds in future
	// following actual message payload
};

/* MSG_SYSTEM_EVENT */
class pitchSystemEvent : public pitchMessage {
public:
	pitchSystemEvent() = default;
	pitchSystemEvent(const eventCode evtCode, const uint16_t trackNo,
			const int64_t timeUs):
		pitchMessage(MSG_SYSTEM_EVENT, 0, trackNo, timeUs),
		eventCode_(evtCode), timeHours_(timeUs/HourUs)
	{
		static_assert(sizeof(*this) <= pMsgSize, "sizeof must less than 64");
	}
	friend bool operator==(const pitchSystemEvent &lhs, const pitchSystemEvent& rhs)
	{
		if (lhs.eventCode_ != rhs.eventCode_
			|| lhs.symbolIndex_ != rhs.symbolIndex_
			|| lhs.trackingNo_ != rhs.trackingNo_) return false;
		return (lhs.timeHours_ == rhs.timeHours_
				&& lhs.timestamp_ == rhs.timestamp_);
	}
	int marshal(void *bufP, int bLen) noexcept
	{
		Serialization	sr(bufP, bLen);
		if (ts3_unlikely(!sr.encode(msgType_, eventCode_, symbolIndex_,
			trackingNo_, timeHours_, timestamp_))) return -1;
		return sr.size();
	}
	bool unmarshal(Serialization &sr) noexcept
	{
		if (ts3_unlikely(!sr.decode(eventCode_, symbolIndex_, trackingNo_,
			timeHours_, timestamp_))) return false;
		msgType_ = MSG_SYSTEM_EVENT;
		return true;
	}
	bool unmarshal(void *bufP, int bLen) noexcept
	{
		Serialization	sr(bufP, bLen);
		return unmarshal(sr);
	}
	eventCode	EventCode() const { return (eventCode)eventCode_; }
	uint32_t	TimeHours() const { return timeHours_; }
	time_t	time() const noexcept {
		time_t res = (time_t)timestamp_ / ts3::duration::us;
		res += (time_t)timeHours_ * 3600;
		return res;
	}
protected:
	u8		eventCode_;	/* PITCH_EVENT_<code> */
	le32	timeHours_;	// hours after 1970/1/1 00:00 UTC
};

/* MSG_SYMBOL_DIRECTORY */
class pitchSymbolDirectory  : public pitchMessage {
public:
	pitchSymbolDirectory() = default;
	pitchSymbolDirectory(const u8 mktC, const string &contr, const u8 classi,
		const u8 prec, const uint16_t symIndex, const uint16_t trkN,
		const int64_t timeUs, const uint32_t lotSize, const uint32_t turnMu,
		const uint32_t lowerL, const uint32_t upperL):
			pitchMessage(MSG_SYMBOL_DIRECTORY, symIndex, trkN, timeUs),
			marketCategory_(mktC),
			classification_(classi), precision_(prec),
			roundLotSize_(lotSize), turnoverMulti_(turnMu),
			lowerLimit_(lowerL), upperLimit_(upperL)
	{
		static_assert(sizeof(*this) <= pMsgSize, "sizeof must less than 64");
		memset(contract_, 0, sizeof(contract_));
		auto ll = contr.size();
		if (ll > sizeof(contract_)) ll = sizeof(contract_);
		memcpy(contract_, contr.data(), ll);
	}
	friend bool operator==(const pitchSymbolDirectory &lhs, const pitchSymbolDirectory& rhs)
	{
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
	int marshal(void *bufP, int bLen) noexcept
	{
		Serialization	sr(bufP, bLen);
		if (ts3_unlikely(!sr.encode(msgType_, marketCategory_))) return -1;
		if (!sr.encodeBytes((const u8*)contract_, sizeof(contract_))) return -1;
		if (!sr.encode(classification_, precision_, symbolIndex_, trackingNo_,
			timestamp_, roundLotSize_, turnoverMulti_, lowerLimit_,
			upperLimit_)) return -1;
		return sr.size();
	}
	bool unmarshal(Serialization &sr) noexcept
	{
		if (ts3_unlikely(!sr.decode1b(marketCategory_))) return false;
		if (!sr.decodeBytes((u8 *)contract_, sizeof(contract_))) return false;
		if (!sr.decode(classification_, precision_, symbolIndex_, trackingNo_,
            timestamp_, roundLotSize_, turnoverMulti_, lowerLimit_,
            upperLimit_)) return false;
		msgType_ = MSG_SYMBOL_DIRECTORY;
		return true;
	}
	bool unmarshal(void *bufP, int bLen) noexcept
	{
		Serialization	sr(bufP, bLen);
		return unmarshal(sr);
	}
	int		Digits() const { return precision_; }
	string	SymbolName() const noexcept {
		size_t sLen=sizeof(contract_);
		if (contract_[sizeof(contract_)-1] == 0) {
			sLen = strlen(contract_);
		}
		return string(contract_, sLen);
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
		const uint16_t symIndex, const uint16_t trkN, const int64_t timeUs) :
		pitchMessage(MSG_SYMBOL_TRADING_ACTION, symIndex, trkN, timeUs),
		tradingState_(state), reason_(reas)
	{
		static_assert(sizeof(*this) <= pMsgSize, "sizeof must less than 64");
	}
	friend bool operator==(const symbolTradingAction &lhs, const symbolTradingAction& rhs)
	{
		if (lhs.tradingState_ != rhs.tradingState_ || lhs.reason_ != rhs.reason_
			|| lhs.symbolIndex_ != rhs.symbolIndex_ 
			|| lhs.trackingNo_ != rhs.trackingNo_) return false;
		return lhs.timestamp_ == rhs.timestamp_;
	}
	int marshal(void *bufP, int bLen) noexcept
	{
		Serialization	sr(bufP, bLen);
		if (ts3_unlikely(!sr.encode(msgType_, tradingState_, reason_,
			symbolIndex_, trackingNo_, timestamp_))) return -1;
		return sr.size();
	}
	bool unmarshal(Serialization &sr) noexcept
	{
		if (ts3_unlikely(!sr.decode(tradingState_, reason_, symbolIndex_,
			trackingNo_, timestamp_))) return false;
		msgType_ = MSG_SYMBOL_TRADING_ACTION;
		return true;
	}
	bool unmarshal(void *bufP, int bLen) noexcept
	{
		Serialization	sr(bufP, bLen);
		return unmarshal(sr);
	}
	tradingState TradingState() const { return (tradingState)tradingState_; }
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
		const uint16_t trkN, const int64_t timeUs, const uint64_t ordRef,
		const int32_t qty, const int32_t prc):
		pitchMessage(MSG_ADD_ORDER, symIndex, trkN, timeUs),
		buySell_(bs), orderRefNo_(ordRef), qty_(qty), price_(prc)
	{
		static_assert(sizeof(*this) <= pMsgSize, "sizeof must less than 64");
	}
	friend bool operator==(const addOrder &lhs, const addOrder& rhs)
	{
		if (lhs.buySell_ != rhs.buySell_ || lhs.symbolIndex_ != rhs.symbolIndex_
			|| lhs.trackingNo_ != rhs.trackingNo_
			|| lhs.timestamp_ != rhs.timestamp_
			|| lhs.orderRefNo_ != rhs.orderRefNo_ ) return false;
		return  lhs.qty_ == rhs.qty_ && lhs.price_ == rhs.price_;
	}
	int marshal(void *bufP, int bLen) noexcept
	{
		Serialization	sr(bufP, bLen);
		if (ts3_unlikely(!sr.encode(msgType_, buySell_, symbolIndex_,
			trackingNo_, timestamp_, orderRefNo_, qty_, price_))) return -1;
		return sr.size();
	}
	bool unmarshal(Serialization &sr) noexcept
	{
		if (ts3_unlikely(!sr.decode(buySell_, symbolIndex_, trackingNo_,
			timestamp_, orderRefNo_, qty_, price_))) return false;
		msgType_ = MSG_ADD_ORDER;
		return true;
	}
	bool unmarshal(void *bufP, int bLen) noexcept
	{
		Serialization	sr(bufP, bLen);
		return unmarshal(sr);
	}
	uint64_t	RefNo() const { return orderRefNo_; }
	bool	isBuy() const {
		return buySell_ == PITCH_BUY_OPEN || buySell_ == PITCH_BUY_COVER;
	}
	bool	isOffset() const {
		return buySell_ == PITCH_BUY_COVER || buySell_ == PITCH_SELL_CLOSE;
	}
	int32_t	Qty() const { return qty_; }
	int32_t	Price() const { return price_; }
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
		const uint16_t trkN, const int64_t timeUs, const uint64_t ordRef,
		const int32_t qty, const uint64_t matchNo) :
		pitchMessage(MSG_ORDER_EXECUTED, symIndex, trkN, timeUs),
		printable_(printA),  orderRefNo_(ordRef),
		qty_(qty), matchNo_(matchNo)
	{
		static_assert(sizeof(*this) <= pMsgSize, "sizeof must less than 64");
	}
	friend bool operator==(const orderExecuted &lhs, const orderExecuted& rhs)
	{
		if (lhs.printable_ != rhs.printable_
			|| lhs.symbolIndex_ != rhs.symbolIndex_
			|| lhs.trackingNo_ != rhs.trackingNo_
			|| lhs.timestamp_ != rhs.timestamp_
			|| lhs.orderRefNo_ != rhs.orderRefNo_ || lhs.qty_ != rhs.qty_)
			return false;
		return lhs.matchNo_ == rhs.matchNo_;
	}
	int marshal(void *bufP, int bLen) noexcept
	{
		Serialization	sr(bufP, bLen);
		if (ts3_unlikely(!sr.encode(msgType_, printable_, symbolIndex_,
			trackingNo_, timestamp_, orderRefNo_, qty_, matchNo_))) return -1;
		return sr.size();
	}
	bool unmarshal(Serialization &sr) noexcept
	{
		if (ts3_unlikely(!sr.decode( printable_, symbolIndex_, trackingNo_,
			timestamp_, orderRefNo_, qty_, matchNo_))) return false;
		msgType_ = MSG_ORDER_EXECUTED;
		return true;
	}
	bool unmarshal(void *bufP, int bLen) noexcept
	{
		Serialization	sr(bufP, bLen);
		return unmarshal(sr);
	}
	bool	Printable() const { return printable_ != 0; }
	uint64_t	RefNo() const { return orderRefNo_; }
	int32_t	Qty() const { return qty_; }
	uint64_t	MatchNo() const { return matchNo_; }
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
		const uint16_t trkN, const int64_t timeUs, const uint64_t ordRef,
		const int32_t qty, const uint64_t matchNo, const int32_t prc) :
		pitchMessage(MSG_ORDER_EXECUTED_WITH_PRICE, symIndex, trkN, timeUs),
		printable_(printA), 
		orderRefNo_(ordRef), qty_(qty), price_(prc), matchNo_(matchNo)
	{
		static_assert(sizeof(*this) <= pMsgSize, "sizeof must less than 64");
	}
	friend bool operator==(const orderEexecutedWithPrice &lhs, const orderEexecutedWithPrice& rhs)
	{
		if (lhs.printable_ != rhs.printable_
			|| lhs.symbolIndex_ != rhs.symbolIndex_
			|| lhs.trackingNo_ != rhs.trackingNo_
			|| lhs.timestamp_ != rhs.timestamp_
			|| lhs.orderRefNo_ != rhs.orderRefNo_ || lhs.qty_ != rhs.qty_)
			return false;
		return lhs.matchNo_ == rhs.matchNo_ && lhs.price_ == rhs.price_;
	}
	int marshal(void *bufP, int bLen) noexcept
	{
		Serialization	sr(bufP, bLen);
		if (ts3_unlikely(!sr.encode(msgType_, printable_, symbolIndex_,
			trackingNo_, timestamp_, orderRefNo_, qty_, matchNo_, price_)))
			return -1;
		return sr.size();
	}
	bool unmarshal(Serialization &sr) noexcept
	{
		if (ts3_unlikely(!sr.decode( printable_, symbolIndex_, trackingNo_,
			timestamp_, orderRefNo_, qty_, matchNo_, price_))) return false;
		msgType_ = MSG_ORDER_EXECUTED_WITH_PRICE;
		return true;
	}
	bool unmarshal(void *bufP, int bLen) noexcept
	{
		Serialization	sr(bufP, bLen);
		return unmarshal(sr);
	}
	bool	Printable() const { return printable_ != 0; }
	uint64_t	RefNo() const { return orderRefNo_; }
	int32_t	Qty() const { return qty_; }
	int32_t	Price() const { return price_; }
	uint64_t	MatchNo() const { return matchNo_; }
protected:
	u8		printable_;
	le64	orderRefNo_;
	le32	qty_;		// executed Qty
	le32	price_;		// executed price
	le64	matchNo_;
};

enum cancelCode : u8 {
	PITCH_CANCEL_BYUSER =	'U',
	PITCH_CANCEL_ARB	=	'A',
	PITCH_CANCEL_MODIFY	=	'M',	// by modify order
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
		const uint16_t trkN, const int64_t timeUs,
		const uint64_t ordRef, const int32_t qty):
		pitchMessage(MSG_ORDER_CANCEL, symIndex, trkN, timeUs),
		cancelReason_(cCode), orderRefNo_(ordRef), qty_(qty)
	{
		static_assert(sizeof(*this) <= pMsgSize, "sizeof must less than 64");
	}
	friend bool operator==(const orderCancel &lhs, const orderCancel& rhs)
	{
		if (lhs.cancelReason_ != rhs.cancelReason_
			|| lhs.symbolIndex_ != rhs.symbolIndex_
			|| lhs.trackingNo_ != rhs.trackingNo_
			|| lhs.timestamp_ != rhs.timestamp_
			|| lhs.orderRefNo_ != rhs.orderRefNo_ ) return false;
		return lhs.qty_ == rhs.qty_;
	}
	int marshal(void *bufP, int bLen) noexcept
	{
		Serialization	sr(bufP, bLen);
		if (!sr.encode(msgType_, cancelReason_, symbolIndex_, trackingNo_,
			timestamp_, orderRefNo_, qty_)) return -1;
		return sr.size();
	}
	bool unmarshal(Serialization &sr) noexcept
	{
		if (ts3_unlikely(!sr.decode( cancelReason_, symbolIndex_, trackingNo_,
			timestamp_, orderRefNo_, qty_))) return false;
		msgType_ = MSG_ORDER_CANCEL;
		return true;
	}
	bool unmarshal(void *bufP, int bLen) noexcept
	{
		Serialization	sr(bufP, bLen);
		return unmarshal(sr);
	}
	uint64_t	RefNo() const { return orderRefNo_; }
	int32_t	Qty() const { return qty_; }
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
		const uint16_t trkN, const int64_t timeUs, const uint64_t ordRef):
		pitchMessage(MSG_ORDER_DELETE, symIndex, trkN, timeUs),
		cancelReason_(cCode), orderRefNo_(ordRef)
	{
		static_assert(sizeof(*this) <= pMsgSize, "sizeof must less than 64");
	}
	friend bool operator==(const orderDelete &lhs, const orderDelete& rhs)
	{
		if (lhs.cancelReason_ != rhs.cancelReason_
			|| lhs.symbolIndex_ != rhs.symbolIndex_
			|| lhs.trackingNo_ != rhs.trackingNo_
			|| lhs.timestamp_ != rhs.timestamp_) return false;
		return lhs.orderRefNo_ == rhs.orderRefNo_;;
	}
	int marshal(void *bufP, int bLen) noexcept
	{
		Serialization	sr(bufP, bLen);
		if (ts3_unlikely(!sr.encode(msgType_, cancelReason_, symbolIndex_,
			trackingNo_, timestamp_, orderRefNo_))) return -1;
		return sr.size();
	}
	bool unmarshal(Serialization &sr) noexcept
	{
		if (ts3_unlikely(!sr.decode( cancelReason_, symbolIndex_, trackingNo_,
			timestamp_, orderRefNo_))) return false;
		msgType_ = MSG_ORDER_DELETE;
		return true;
	}
	bool unmarshal(void *bufP, int bLen) noexcept
	{
		Serialization	sr(bufP, bLen);
		return unmarshal(sr);
	}
	uint64_t	RefNo() const { return orderRefNo_; }
protected:
	u8		cancelReason_;
	le64	orderRefNo_;
};

/* MSG_ORDER_REPLACE */
class orderReplace  : public pitchMessage {
public:
	orderReplace() = default;
	orderReplace(const uint16_t symIndex, const uint16_t trkN,
		const int64_t timeUs, const uint64_t ordRef, const uint64_t NewOrdRef,
		const int32_t qty, const int32_t prc):
		pitchMessage(MSG_ORDER_REPLACE, symIndex, trkN, timeUs),
		orderRefNo_(ordRef), newOrderRefNo_(NewOrdRef), qty_(qty), price_(prc)
	{
		static_assert(sizeof(*this) <= pMsgSize, "sizeof must less than 64");
	}
	friend bool operator==(const orderReplace &lhs, const orderReplace& rhs)
	{
		if (lhs.symbolIndex_ != rhs.symbolIndex_
			|| lhs.trackingNo_ != rhs.trackingNo_
			|| lhs.timestamp_ != rhs.timestamp_
			|| lhs.orderRefNo_ != rhs.orderRefNo_
			|| lhs.newOrderRefNo_ != rhs.newOrderRefNo_
			|| lhs.qty_ != rhs.qty_) return false;
		return lhs.price_ == rhs.price_;
	}
	int marshal(void *bufP, int bLen) noexcept
	{
		Serialization	sr(bufP, bLen);
		if (ts3_unlikely(!sr.encode(msgType_, symbolIndex_, trackingNo_,
			timestamp_, orderRefNo_, newOrderRefNo_, qty_, price_))) return -1;
		return sr.size();
	}
	bool unmarshal(Serialization &sr) noexcept
	{
		if (ts3_unlikely(!sr.decode( symbolIndex_, trackingNo_, timestamp_,
			orderRefNo_, newOrderRefNo_, qty_, price_))) return false;
		msgType_ = MSG_ORDER_REPLACE;
		return true;
	}
	bool unmarshal(void *bufP, int bLen) noexcept
	{
		Serialization	sr(bufP, bLen);
		return unmarshal(sr);
	}
	uint64_t	RefNo() const { return orderRefNo_; }
	uint64_t	newRefNo() const { return newOrderRefNo_; }
	int32_t	Qty() const { return qty_; }
	int32_t	Price() const { return price_; }
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
		const uint16_t trkN, const int64_t timeUs, const uint64_t ordRef,
		const int32_t qty, const int32_t prc, const uint64_t mNo):
		pitchMessage(MSG_TRADE, symIndex, trkN, timeUs),
		buySell_(bs), orderRefNo_(ordRef), qty_(qty), price_(prc),
		matchNo_(mNo)
	{
		static_assert(sizeof(*this) <= pMsgSize, "sizeof must less than 64");
	}
	friend bool operator==(const msgTrade &lhs, const msgTrade& rhs)
	{
		if (lhs.buySell_ != rhs.buySell_ || lhs.symbolIndex_ != rhs.symbolIndex_
			|| lhs.trackingNo_ != rhs.trackingNo_
			|| lhs.timestamp_ != rhs.timestamp_
			|| lhs.orderRefNo_ != rhs.orderRefNo_
			|| lhs.qty_ != rhs.qty_) return false;
		return lhs.price_ == rhs.price_ && lhs.matchNo_ == rhs.matchNo_;
	}
	int marshal(void *bufP, int bLen) noexcept
	{
		Serialization	sr(bufP, bLen);
		if (ts3_unlikely(!sr.encode(msgType_, buySell_, symbolIndex_,
			trackingNo_, timestamp_, orderRefNo_, qty_, price_, matchNo_)))
			return -1;
		return sr.size();
	}
	bool unmarshal(Serialization &sr) noexcept
	{
		if (ts3_unlikely(!sr.decode( buySell_, symbolIndex_, trackingNo_,
			timestamp_, orderRefNo_, qty_, price_, matchNo_))) return false;
		msgType_ = MSG_TRADE;
		return true;
	}
	bool unmarshal(void *bufP, int bLen) noexcept
	{
		Serialization	sr(bufP, bLen);
		return unmarshal(sr);
	}
	bool	isBuy() const {
		return buySell_ == PITCH_BUY_OPEN || buySell_ == PITCH_BUY_COVER;
	}
	bool	isOffset() const {
		return buySell_ == PITCH_BUY_COVER || buySell_ == PITCH_SELL_CLOSE;
	}
	uint64_t	RefNo() const { return orderRefNo_; }
	int32_t	Qty() const { return qty_; }
	int32_t	Price() const { return price_; }
	uint64_t	MatchNo() const { return matchNo_; }
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
	crossTrade(const u8 ct, const uint16_t symIndex, const uint16_t trkN,
		const int64_t timeUs, const int32_t qty, const int32_t prc,
		const int32_t pclose, const int32_t op, const uint64_t mNo):
		pitchMessage(MSG_CROSS_TRADE, symIndex, trkN, timeUs),
		crossType_(ct), qty_(qty), price_(prc), pclose_(pclose),
		openInterest_(op), matchNo_(mNo)
	{
		static_assert(sizeof(*this) <= pMsgSize, "sizeof must less than 64");
	}
	friend bool operator==(const crossTrade &lhs, const crossTrade& rhs)
	{
		if (lhs.crossType_ != rhs.crossType_
			|| lhs.symbolIndex_ != rhs.symbolIndex_
			|| lhs.trackingNo_ != rhs.trackingNo_
			|| lhs.timestamp_ != rhs.timestamp_
			|| lhs.qty_ != rhs.qty_ || lhs.price_ != rhs.price_
			|| lhs.pclose_ != rhs.pclose_
			|| lhs.openInterest_ != rhs.openInterest_) return false;
		return  lhs.matchNo_ == rhs.matchNo_;
	}
	int marshal(void *bufP, int bLen) noexcept
	{
		Serialization	sr(bufP, bLen);
		if (ts3_unlikely(!sr.encode(msgType_, crossType_, symbolIndex_,
			trackingNo_, timestamp_, qty_, price_, pclose_, openInterest_,
			matchNo_))) return -1;
		return sr.size();
	}
	bool unmarshal(Serialization &sr) noexcept
	{
		if (ts3_unlikely(!sr.decode(crossType_, symbolIndex_, trackingNo_,
		timestamp_, qty_, price_, pclose_, openInterest_, matchNo_)))
			return false;
		msgType_ = MSG_CROSS_TRADE;
		return true;
	}
	bool unmarshal(void *bufP, int bLen) noexcept
	{
		Serialization	sr(bufP, bLen);
		return unmarshal(sr);
	}
	int32_t	Qty() const { return qty_; }
	int32_t	Price() const { return price_; }
	int32_t	Pclose() const { return pclose_; }
	int32_t	OpenInterest() const { return openInterest_; }
	uint64_t	MatchNo() const { return matchNo_; }
protected:
	u8		crossType_;
	le32	qty_;
	le32	price_;
	le32	pclose_;
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

#ifdef	_NTEST
inline	std::shared_ptr<pitchMessage>
unmarshal(const void *bufP, const int bLen) noexcept
{
	Serialization	sr(bufP, bLen);
	std::shared_ptr<pitchMessage>	ret;
	u8		msgT;
	if (ts3_unlikely(!sr.decode(msgT))) return ret;
	switch((msgType)msgT) {
	case MSG_SYSTEM_EVENT:
	{
		auto res = std::make_shared<pitchSystemEvent>();
		ret = static_pointer_cast<pitchMessage>(res);
	}
		break;
	case MSG_SYMBOL_DIRECTORY:
	{
		auto res = std::make_shared<pitchSymbolDirectory>();
		ret = static_pointer_cast<pitchMessage>(res);
	}
		break;
	case MSG_SYMBOL_TRADING_ACTION:
	{
		auto res = std::make_shared<symbolTradingAction>();
		ret = static_pointer_cast<pitchMessage>(res);
	}
		break;
	case MSG_ADD_ORDER:
	{
		auto res = std::make_shared<addOrder>();
		ret = static_pointer_cast<pitchMessage>(res);
	}
		break;
	case MSG_ORDER_EXECUTED:
	{
		auto res = std::make_shared<orderExecuted>();
		ret = static_pointer_cast<pitchMessage>(res);
	}
		break;
	case MSG_ORDER_EXECUTED_WITH_PRICE:
	{
		auto res = std::make_shared<orderEexecutedWithPrice>();
		ret = static_pointer_cast<pitchMessage>(res);
	}
		break;
	case MSG_ORDER_CANCEL:
	{
		auto res = std::make_shared<orderCancel>();
		ret = static_pointer_cast<pitchMessage>(res);
	}
		break;
	case MSG_ORDER_DELETE:
	{
		auto res = std::make_shared<orderDelete>();
		ret = static_pointer_cast<pitchMessage>(res);
	}
		break;
	case MSG_ORDER_REPLACE:
	{
		auto res = std::make_shared<orderReplace>();
		ret = static_pointer_cast<pitchMessage>(res);
	}
		break;
	case MSG_TRADE:
	{
		auto res = std::make_shared<msgTrade>();
		ret = static_pointer_cast<pitchMessage>(res);
	}
		break;
	case MSG_CROSS_TRADE:
	{
		auto res = std::make_shared<crossTrade>();
		ret = static_pointer_cast<pitchMessage>(res);
	}
		break;
	case MSG_NOII:
	case MSG_BROKEN_TRADE:
		return ret;
	default:
		// NO WAY GO HERE
		return ret;
	}
	if (ts3_unlikely(!ret->unmarshal(sr))) {
		ret = std::shared_ptr<pitchMessage>();
	}
	return ret;
}


template<typename T>T
unmarshal(const void *bufP, const int bLen, void *outBufP) noexcept
{
	Serialization	sr(bufP, bLen);
	T	ret=nullptr;
	u8		msgT;
	if (ts3_unlikely(!sr.decode(msgT))) return ret;
	switch((msgType)msgT) {
	case MSG_SYSTEM_EVENT:
	{
		auto res = new (outBufP) pitchSystemEvent;
		//ret = reinterpret_cast<T>(res);
		ret = res;
	}
		break;
	case MSG_SYMBOL_DIRECTORY:
	{
		auto res =  new(outBufP) pitchSymbolDirectory;
		//ret = reinterpret_cast<T>(res);
		ret = res;
	}
		break;
	case MSG_SYMBOL_TRADING_ACTION:
	{
		auto res = new (outBufP) symbolTradingAction;
		//ret = reinterpret_cast<T>(res);
		ret = res;
	}
		break;
	case MSG_ADD_ORDER:
	{
		auto res = new (outBufP) addOrder;
		//ret = reinterpret_cast<T>(res);
		ret = res;
	}
		break;
	case MSG_ORDER_EXECUTED:
	{
		auto res = new (outBufP) orderExecuted;
		//ret = reinterpret_cast<T>(res);
		ret = res;
	}
		break;
	case MSG_ORDER_EXECUTED_WITH_PRICE:
	{
		auto res = new (outBufP) orderEexecutedWithPrice;
		//ret = reinterpret_cast<T>(res);
		ret = res;
	}
		break;
	case MSG_ORDER_CANCEL:
	{
		auto res = new (outBufP) orderCancel;
		//ret = reinterpret_cast<T>(res);
		ret = res;
	}
		break;
	case MSG_ORDER_DELETE:
	{
		auto res = new (outBufP) orderDelete;
		//ret = reinterpret_cast<T>(res);
		ret = res;
	}
		break;
	case MSG_ORDER_REPLACE:
	{
		auto res = new (outBufP) orderReplace;
		//ret = reinterpret_cast<T>(res);
		ret = res;
	}
		break;
	case MSG_TRADE:
	{
		auto res = new (outBufP) msgTrade;
		//ret = reinterpret_cast<T>(res);
		ret = res;
	}
		break;
	case MSG_CROSS_TRADE:
	{
		auto res = new (outBufP) crossTrade;
		//ret = reinterpret_cast<T>(res);
		ret = res;
	}
		break;
	case MSG_NOII:
	case MSG_BROKEN_TRADE:
		return ret;
	default:
		// NO WAY GO HERE
		return ret;
	}
	if (ts3_unlikely(!ret->unmarshal(sr))) {
		return nullptr;
	}
	return ret;
}
#endif

}
#endif	// __TS3_PITCH_HPP__

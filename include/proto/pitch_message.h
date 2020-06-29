#pragma once

#ifndef	__TS3_PITCH_MESSAGE_H__
#define	__TS3_PITCH_MESSAGE_H__

#include "ts3/types.h"

/*
 * Message types:
 */
enum pitch_msg_type {
	PITCH_MSG_SYSTEM_EVENT			= 'S',	/* Section 4.1. */
	PITCH_MSG_SYMBOL_DIRECTORY		= 'R',	/* Section 4.2.1. */
	PITCH_MSG_SYMBOL_TRADING_ACTION	= 'H',	/* Section 4.2.2. */
	PITCH_MSG_ADD_ORDER				= 'A',	/* Section 4.3.1. */
	PITCH_MSG_ORDER_EXECUTED		= 'E',	/* Section 4.4.1. */
	PITCH_MSG_ORDER_EXECUTED_WITH_PRICE	= 'C',	/* Section 4.4.2. */
	PITCH_MSG_ORDER_CANCEL			= 'X',	/* Section 4.4.3. */
	PITCH_MSG_ORDER_DELETE			= 'D',	/* Section 4.4.4. */
	PITCH_MSG_ORDER_REPLACE			= 'U',	/* Section 4.4.5. */
	PITCH_MSG_TRADE					= 'P',	/* Section 4.5.1. */
	PITCH_MSG_CROSS_TRADE			= 'Q',	/* Section 4.5.2. */
	PITCH_MSG_BROKEN_TRADE			= 'B',	/* Section 4.5.3. */
	PITCH_MSG_NOII					= 'I',	/* Section 4.6. */
};

/*
 * System event codes:
 */
enum pitch_event_code {
	PITCH_EVENT_START_OF_MESSAGES		= 'O',
	PITCH_EVENT_START_OF_SYSTEM_HOURS	= 'S',
	PITCH_EVENT_START_OF_MARKET_HOURS	= 'Q',
	PITCH_EVENT_END_OF_MARKET_HOURS		= 'M',
	PITCH_EVENT_END_OF_SYSTEM_HOURS		= 'E',
	PITCH_EVENT_END_OF_MESSAGES			= 'C',
	PITCH_EVENT_EMERGENCY_HALT			= 'A',
	PITCH_EVENT_EMERGENCY_QUOTE_ONLY	= 'R',
	PITCH_EVENT_EMERGENCY_RESUMPTION	= 'B',
};

/*
 * A data structure for PITCH messages.
 */
struct pitch_message {
	u8			MessageType;
};

/* PITCH_MSG_SYSTEM_EVENT */
struct pitch_msg_system_event {
	u8			MessageType;
	char		EventCode;	/* PITCH_EVENT_<code> */
	le16		SymbolIndex;	// always 0
	le16		TrackingNumber;
	le32		TimeHours;	// hours after 1970/1/1 00:00 UTC
	le32		Timestamp;
} __attribute__((packed));

/* PITCH_MSG_SYMBOL_DIRECTORY */
struct pitch_msg_symbol_directory {
	u8			MessageType;
	char		MarketCategory;
	char		Contract[16];
	u8			Classification;
	u8			Precision;	// digits after dot
	le16		SymbolIndex;
	le16		TrackingNumber;
	le32		Timestamp;
	le32		RoundLotSize;
	le32		TurnoverMulti;
	le32		lower_limit;
	le32		upper_limit;
} __attribute__((packed));

enum pitch_trading_state {
	PITCH_TRADE_HALT	= 'H',
	PITCH_TRADE_PREAUCTION = 'P',
	PITCH_TRADE_AUCTION	= 'A',
	PITCH_TRADE_PAUSE	= 'U',
	PITCH_TRADE_NORMAL	= 'C',
	PITCH_TRADE_BREAK	= 'B',
};

/* PITCH_MSG_SYMBOL_TRADING_ACTION */
struct pitch_msg_symbol_trading_action {
	u8		MessageType;
	char	TradingState;
	le16	Reason;
	le16	SymbolIndex;
	le16	TrackingNumber;
	le32	Timestamp;
} __attribute__((packed));

enum pitch_BuySell_indicator {
	PITCH_BUY_OPEN			= 'B',
	PITCH_SELL_OPEN			= 'S',
	PITCH_BUY_COVER			= 'C',
	PITCH_SELL_CLOSE		= 'O',
};

/* PITCH_MSG_ADD_ORDER */
struct pitch_msg_add_order {
	u8		MessageType;
	char	BuySellIndicator;
	le16	SymbolIndex;
	le16	TrackingNumber;
	le32	Timestamp;
	le64	OrderReferenceNumber;
	le32	Qty;
	le32	Price;
} __attribute__((packed));

/* PITCH_MSG_ORDER_EXECUTED */
struct pitch_msg_order_executed {
	u8		MessageType;
	char	Printable;
	le16	SymbolIndex;
	le16	TrackingNumber;
	le32	Timestamp;
	le64	OrderReferenceNumber;
	le32	ExecutedQty;
	le64	MatchNumber;
} __attribute__((packed));

/* PITCH_MSG_ORDER_EXECUTED_WITH_PRICE */
struct pitch_msg_order_executed_with_price {
	u8		MessageType;
	char	Printable;
	le16	SymbolIndex;
	le16	TrackingNumber;
	le32	Timestamp;
	le64	OrderReferenceNumber;
	le32	ExecutedQty;
	le64	MatchNumber;
	le32	ExecutionPrice;
} __attribute__((packed));

enum pitch_cancel_code {
	PITCH_CANCEL_BYUSER =	'U',
	PITCH_CANCEL_ARB	=	'A',
	PITCH_CANCEL_MODIFY =   'M',    // by modify order
	PITCH_CANCEL_ODDLOT	=	'O',	// not normalization lots
	PITCH_CANCEL_OOB	=	'B',	// out of price band
	PITCH_CANCEL_BREAK	=	'S',	// broken session
	PITCH_CANCEL_NORMAL	=	'N',	// order cancel out of normal trading
};

/* PITCH_MSG_ORDER_CANCEL */
struct pitch_msg_order_cancel {
	u8		MessageType;
	char	CancelReason;
	le16	SymbolIndex;
	le16	TrackingNumber;
	le32	Timestamp;
	le64	OrderReferenceNumber;
	le32	CanceledQty;
} __attribute__((packed));

/* PITCH_MSG_ORDER_DELETE */
struct pitch_msg_order_delete {
	u8		MessageType;
	char	CancelReason;
	le16	SymbolIndex;
	le16	TrackingNumber;
	le32	Timestamp;
	le64	OrderReferenceNumber;
} __attribute__((packed));

/* PITCH_MSG_ORDER_REPLACE */
struct pitch_msg_order_replace {
	u8		MessageType;
	le16	SymbolIndex;
	le16	TrackingNumber;
	le32	Timestamp;
	le64	OriginalOrderReferenceNumber;
	le64	NewOrderReferenceNumber;
	le32	Qty;
	le32	Price;
} __attribute__((packed));

/* PITCH_MSG_TRADE */
struct pitch_msg_trade {
	u8		MessageType;
	char	BuySellIndicator;
	le16	SymbolIndex;
	le16	TrackingNumber;
	le32	Timestamp;
	le64	OrderReferenceNumber;
	le32	Qty;
	le32	Price;
	le64	MatchNumber;
} __attribute__((packed));

/* PITCH_MSG_CROSS_TRADE */
struct pitch_msg_cross_trade {
	u8		MessageType;
	char	CrossType;
	le16	SymbolIndex;
	le16	TrackingNumber;
	le32	Timestamp;
	le32	Qty;
	le32	CrossPrice;
	le32	PrevClose;
	le32	OpenInterest;
	le64	MatchNumber;
} __attribute__((packed));

/* PITCH_MSG_BROKEN_TRADE */
struct pitch_msg_broken_trade {
	u8		MessageType;
	char	BrokenTag;
	le16	SymbolIndex;
	le16	TrackingNumber;
	le32	Timestamp;
	le64	MatchNumber;
} __attribute__((packed));

/* PITCH_MSG_NOII */
struct pitch_msg_noii {
	u8		MessageType;
	char	ImbalanceDirection;
	le16	SymbolIndex;
	le16	TrackingNumber;
	le32	Timestamp;
	le64	PairedQty;
	le64	ImbalanceQty;
	le32	FarPrice;
	le32	NearPrice;
	le32	CurrentReferencePrice;
	char	CrossType;
	char	PriceVariationIndicator;
} __attribute__((packed));

#endif	// __TS3_PITCH_MESSAGE_H__

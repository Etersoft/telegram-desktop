/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

namespace Storage {

struct SparseIdsListQuery {
	SparseIdsListQuery(
		MsgId aroundId,
		int limitBefore,
		int limitAfter)
	: aroundId(aroundId)
	, limitBefore(limitBefore)
	, limitAfter(limitAfter) {
	}

	MsgId aroundId = 0;
	int limitBefore = 0;
	int limitAfter = 0;

};

struct SparseIdsListResult {
	base::optional<int> count;
	base::optional<int> skippedBefore;
	base::optional<int> skippedAfter;
	base::flat_set<MsgId> messageIds;
};

struct SparseIdsSliceUpdate {
	const base::flat_set<MsgId> *messages = nullptr;
	MsgRange range;
	base::optional<int> count;
};

class SparseIdsList {
public:
	void addNew(MsgId messageId);
	void addExisting(MsgId messageId, MsgRange noSkipRange);
	void addSlice(
		std::vector<MsgId> &&messageIds,
		MsgRange noSkipRange,
		base::optional<int> count);
	void removeOne(MsgId messageId);
	void removeAll();
	rpl::producer<SparseIdsListResult> query(SparseIdsListQuery &&query) const;
	rpl::producer<SparseIdsSliceUpdate> sliceUpdated() const;

private:
	struct Slice {
		Slice(base::flat_set<MsgId> &&messages, MsgRange range);

		template <typename Range>
		void merge(const Range &moreMessages, MsgRange moreNoSkipRange);

		base::flat_set<MsgId> messages;
		MsgRange range;

		inline bool operator<(const Slice &other) const {
			return range.from < other.range.from;
		}

	};

	template <typename Range>
	int uniteAndAdd(
		SparseIdsSliceUpdate &update,
		base::flat_set<Slice>::iterator uniteFrom,
		base::flat_set<Slice>::iterator uniteTill,
		const Range &messages,
		MsgRange noSkipRange);
	template <typename Range>
	int addRangeItemsAndCountNew(
		SparseIdsSliceUpdate &update,
		const Range &messages,
		MsgRange noSkipRange);
	template <typename Range>
	void addRange(
		const Range &messages,
		MsgRange noSkipRange,
		base::optional<int> count,
		bool incrementCount = false);

	SparseIdsListResult queryFromSlice(
		const SparseIdsListQuery &query,
		const Slice &slice) const;

	base::optional<int> _count;
	base::flat_set<Slice> _slices;

	rpl::event_stream<SparseIdsSliceUpdate> _sliceUpdated;

};

} // namespace Storage
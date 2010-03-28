#pragma once

#include <boost/shared_ptr.hpp>

#include <QAbstractItemModel>
#include <QList>

#include "Swiften/MUC/MUCBookmark.h"

#include "Swift/QtUI/ChatList/ChatListGroupItem.h"

namespace Swift {
	class ChatListModel : public QAbstractItemModel {
		Q_OBJECT
		public:
			ChatListModel();
			void addMUCBookmark(boost::shared_ptr<MUCBookmark> bookmark);
			void removeMUCBookmark(boost::shared_ptr<MUCBookmark> bookmark);
			int columnCount(const QModelIndex& parent = QModelIndex()) const;
			QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
			QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const;
			QModelIndex parent(const QModelIndex& index) const;
			int rowCount(const QModelIndex& parent = QModelIndex()) const;
		private:
			ChatListGroupItem* mucBookmarks_;
			ChatListGroupItem* root_;
	};

}

#ifndef MOTIV_TRACEDATAMODEL_HPP
#define MOTIV_TRACEDATAMODEL_HPP


#include <QtCore/QObject>
#include <QDebug>
#include "src/models/filetrace.hpp"

class TraceDataModel : public QObject {
    Q_OBJECT

public: // constructors
    explicit TraceDataModel(std::shared_ptr<FileTrace> trace, QObject *parent = nullptr)
    : fullTrace(trace), end(trace->getRuntime()) {
        updateSelection();
    }

public: Q_SIGNALS:
    void selectionUpdated();

public Q_SLOTS:
    void setBegin(size_t newBegin) {
        setBegin(otf2::chrono::duration(newBegin));
    }

    void setEnd(size_t newEnd) {
        setEnd(otf2::chrono::duration(newEnd));
    }

    void setPeriod(size_t newBegin, size_t newEnd) {
        setPeriod(otf2::chrono::duration(newBegin), otf2::chrono::duration(newEnd));
    }

public: //methods
    void setBegin(otf2::chrono::duration newBegin) {
        assert(newBegin < fullTrace->getRuntime());
        assert(newBegin <= end);
        begin = newBegin;

        updateSelection();
    }

    void setEnd(otf2::chrono::duration newEnd) {
        assert(newEnd < fullTrace->getRuntime());
        assert(newEnd >= begin);
        end = newEnd;

        updateSelection();
    }

    void setPeriod(otf2::chrono::duration newBegin, otf2::chrono::duration newEnd) {
        assert(newBegin <= newEnd);
        assert(newBegin <= fullTrace->getRuntime());
        assert(newEnd <= fullTrace->getRuntime());
        begin = newBegin;
        end = newEnd;

        updateSelection();
    }

    size_t getBegin() const {
        return begin.count();
    }

    size_t getEnd() const {
        return end.count();
    }

    std::shared_ptr<Trace> getSelection() const {
        return selection;
    }

protected: //methods
    void updateSelection() {
        selection = std::static_pointer_cast<SubTrace>(fullTrace->subtrace(begin, end));
        Q_EMIT selectionUpdated();
        qDebug() << "Selection updated";
    }

private: // data
    std::shared_ptr<FileTrace> fullTrace = nullptr;
    std::shared_ptr<SubTrace> selection = nullptr;
    otf2::chrono::duration begin{0};
    otf2::chrono::duration end{0};

};


#endif //MOTIV_TRACEDATAMODEL_HPP
#ifndef DCACHE_H
#define DCACHE_H

#ifndef PROCESSOR_H
#error This file should be included in Processor.h
#endif

class DCache : public Object, public Inspect::Interface<Inspect::Read>
{
public:
    /// The state of a cache-line
    enum LineState
    {
        LINE_EMPTY,      ///< Line is empty.
        LINE_LOADING,    ///< Line is being loaded.
        LINE_INVALID,    ///< Line is invalid.
        LINE_FULL        ///< Line is full.
    };

    struct Line
    {
        LineState   state;      ///< The line state.
        bool        processing; ///< Has the line been added to m_returned yet?
        MemAddr     tag;        ///< The address tag.
        char*       data;       ///< The data in this line.
        bool*       valid;      ///< A bitmap of valid bytes in this line.
        CycleNo     access;     ///< Last access time of this line (for LRU).
        RegAddr     waiting;    ///< First register waiting on this line.
    };

private:
    struct Request
    {
        MemAddr address;
        bool    write;
        MemData data;
        TID     tid;
    };
    
    // Information for multi-register writes
    struct WritebackState
    {
        LFID         fid;    ///< FID of the thread's that's waiting on the register
        RegAddr      addr;   ///< Address of the next register to write
        unsigned int size;   ///< Number of registers remaining to write
        unsigned int offset; ///< Current offset in the multi-register operand
        uint64_t     value;  ///< Value to write
        RegAddr      next;   ///< Next register after this one
    };
    
    Result FindLine(MemAddr address, Line* &line, bool check_only);

    Processor&           m_parent;          ///< Parent processor.
    Allocator&			 m_allocator;       ///< Allocator component.
	FamilyTable&		 m_familyTable;     ///< Family table .
	RegisterFile&		 m_regFile;         ///< Register File.
    std::vector<Line>    m_lines;           ///< The cache-lines.
	size_t               m_assoc;           ///< Config: Cache associativity.
	size_t               m_sets;            ///< Config: Number of sets in the cace.
	size_t               m_lineSize;        ///< Config: Size of a cache line, in bytes.
    Buffer<CID>          m_returned;        ///< Returned cache-lines waiting to be processed.
    Buffer<TID>          m_completedWrites; ///< Completed writes.
    Buffer<Request>      m_outgoing;        ///< Outgoing buffer to memory bus.
    WritebackState       m_wbstate;         ///< Writeback state
    uint64_t             m_numHits;         ///< Number of hits so far.
    uint64_t             m_numMisses;       ///< Number of misses so far.
       
    Result DoCompletedReads();
    Result DoCompletedWrites();
    Result DoOutgoingRequests();

public:
    DCache(const std::string& name, Processor& parent, Clock& clock, Allocator& allocator, FamilyTable& familyTable, RegisterFile& regFile, const Config& config);
    ~DCache();
    
    // Processes
    Process p_IncomingReads;
    Process p_IncomingWrites;
    Process p_Outgoing;

    ArbitratedService<> p_service;

    // Public interface
    Result Read (MemAddr address, void* data, MemSize size, LFID fid, RegAddr* reg);
    Result Write(MemAddr address, void* data, MemSize size, LFID fid, TID tid);

    size_t GetLineSize() const { return m_lineSize; }

    // Memory callbacks
    bool OnMemoryReadCompleted(MemAddr addr, const MemData& data);
    bool OnMemoryWriteCompleted(TID tid);
    bool OnMemorySnooped(MemAddr addr, const MemData& data);
    bool OnMemoryInvalidated(MemAddr addr);

    // Debugging
    void Cmd_Help(std::ostream& out, const std::vector<std::string>& arguments) const;
    void Cmd_Read(std::ostream& out, const std::vector<std::string>& arguments) const;

    size_t GetAssociativity() const { return m_assoc; }
    size_t GetNumLines()      const { return m_lines.size(); }
    size_t GetNumSets()       const { return m_sets; }


    const Line& GetLine(size_t i) const { return m_lines[i];  }
};

#endif

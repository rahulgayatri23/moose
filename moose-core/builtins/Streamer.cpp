/***
 *       Filename:  Streamer.cpp
 *
 *    Description:  Stream table data.
 *
 *        Version:  0.0.1
 *        Created:  2016-04-26

 *       Revision:  none
 *
 *         Author:  Dilawar Singh <dilawars@ncbs.res.in>
 *   Organization:  NCBS Bangalore
 *
 *        License:  GNU GPL2
 */

#include <algorithm>
#include <sstream>

#include "global.h"
#include "header.h"
#include "Streamer.h"
#include "Clock.h"
#include "utility/utility.h"
#include "../shell/Shell.h"


const Cinfo* Streamer::initCinfo()
{
    /*-----------------------------------------------------------------------------
     * Finfos
     *-----------------------------------------------------------------------------*/
    static ValueFinfo< Streamer, string > outfile(
        "outfile"
        , "File/stream to write table data to. Default is is __moose_tables__.dat.n"
        " By default, this object writes data every second \n"
        , &Streamer::setOutFilepath
        , &Streamer::getOutFilepath
    );

    static ValueFinfo< Streamer, string > format(
        "format"
        , "Format of output file, default is csv"
        , &Streamer::setFormat
        , &Streamer::getFormat
    );

    static ReadOnlyValueFinfo< Streamer, size_t > numTables (
        "numTables"
        , "Number of Tables handled by Streamer "
        , &Streamer::getNumTables
    );

    /*-----------------------------------------------------------------------------
     *
     *-----------------------------------------------------------------------------*/
    static DestFinfo process(
        "process"
        , "Handle process call"
        , new ProcOpFunc< Streamer >( &Streamer::process )
    );

    static DestFinfo reinit(
        "reinit"
        , "Handles reinit call"
        , new ProcOpFunc< Streamer > ( &Streamer::reinit )
    );


    static DestFinfo addTable(
        "addTable"
        , "Add a table to Streamer"
        , new OpFunc1<Streamer, Id>( &Streamer::addTable )
    );

    static DestFinfo addTables(
        "addTables"
        , "Add many tables to Streamer"
        , new OpFunc1<Streamer, vector<Id> >( &Streamer::addTables )
    );

    static DestFinfo removeTable(
        "removeTable"
        , "Remove a table from Streamer"
        , new OpFunc1<Streamer, Id>( &Streamer::removeTable )
    );

    static DestFinfo removeTables(
        "removeTables"
        , "Remove tables -- if found -- from Streamer"
        , new OpFunc1<Streamer, vector<Id> >( &Streamer::removeTables )
    );

    /*-----------------------------------------------------------------------------
     *  ShareMsg definitions.
     *-----------------------------------------------------------------------------*/
    static Finfo* procShared[] =
    {
        &process , &reinit , &addTable, &addTables, &removeTable, &removeTables
    };

    static SharedFinfo proc(
        "proc",
        "Shared message for process and reinit",
        procShared, sizeof( procShared ) / sizeof( const Finfo* )
    );

    static Finfo * tableStreamFinfos[] =
    {
        &outfile, &format, &proc, &numTables
    };

    static string doc[] =
    {
        "Name", "Streamer",
        "Author", "Dilawar Singh, 2016, NCBS, Bangalore.",
        "Description", "Streamer: Stream moose.Table data to out-streams\n"
    };

    static Dinfo< Streamer > dinfo;

    static Cinfo tableStreamCinfo(
        "Streamer",
        TableBase::initCinfo(),
        tableStreamFinfos,
        sizeof( tableStreamFinfos )/sizeof(Finfo *),
        &dinfo,
        doc,
        sizeof(doc) / sizeof(string)
    );

    return &tableStreamCinfo;
}

static const Cinfo* tableStreamCinfo = Streamer::initCinfo();

// Class function definitions

Streamer::Streamer() 
{
    // Not all compilers allow initialization during the declaration of class
    // methods.
    format_ = "npy";
    columns_.push_back( "time" );               /* First column is time. */
    tables_.resize(0);
    tableIds_.resize(0);
    tableTick_.resize(0);
    tableDt_.resize(0);
    data_.resize(0);
}

Streamer& Streamer::operator=( const Streamer& st )
{
    return *this;
}


Streamer::~Streamer()
{
    cleanUp();
}

void Streamer::cleanUp( void )
{
    /*  Write the left-overs. */
    zipWithTime( data_, currTime_ );
    StreamerBase::writeToOutFile( outfilePath_, format_, "a", data_, columns_ );
}

/**
 * @brief Reinit.
 *
 * @param e
 * @param p
 */
void Streamer::reinit(const Eref& e, ProcPtr p)
{

    if( tables_.size() == 0 )
    {
        moose::showWarn( "Zero tables in streamer. Disabling Streamer" );
        e.element()->setTick( -2 );             /* Disable process */
        return;
    }

    Clock* clk = reinterpret_cast<Clock*>( Id(1).eref().data() );
    for (size_t i = 0; i < tableIds_.size(); i++) 
    {
        int tickNum = tableIds_[i].element()->getTick();
        double tick = clk->getTickDt( tickNum );
        tableDt_.push_back( tick );
        // Make sure that all tables have the same tick.
        if( i > 0 )
        {
            if( tick != tableDt_[0] )
            {
                moose::showWarn( "Table " + tableIds_[i].path() + " has "
                        " different clock dt. "
                        " Make sure all tables added to Streamer have the same "
                        " dt value."
                        );
            }
        }
    }


    // Push each table dt_ into vector of dt
    for( size_t i = 0; i < tables_.size(); i++)
    {
        Id tId = tableIds_[i];
        int tickNum = tId.element()->getTick();
        tableDt_.push_back( clk->getTickDt( tickNum ) );
    }


    // Make sure all tables have same dt_ else disable the streamer.
    vector<unsigned int> invalidTables;
    for (size_t i = 1; i < tableTick_.size(); i++) 
    {
        if( tableTick_[i] != tableTick_[0] )
        {
            LOG( moose::warning
                    , "Table " << tableIds_[i].path()
                    << " has tick (dt) which is different than the first table."
                    << endl 
                    << " Got " << tableTick_[i] << " expected " << tableTick_[0]
                    << endl << " Disabling this table."
                    );
            invalidTables.push_back( i );
        }
    }

    for (size_t i = 0; i < invalidTables.size(); i++) 
    {
        tables_.erase( tables_.begin() + i );
        tableDt_.erase( tableDt_.begin() + i );
        tableIds_.erase( tableIds_.begin() + i );
    }

    if( ! isOutfilePathSet_ )
    {
        string defaultPath = "_tables/" + moose::moosePathToUserPath( e.id().path() );
        setOutFilepath( defaultPath );
    }
    currTime_ = 0.0;
    // Prepare data.
    zipWithTime( data_, currTime_ );
    StreamerBase::writeToOutFile( outfilePath_, format_, "w", data_, columns_);
    data_.clear();
}

/**
 * @brief This function is called at its clock tick.
 *
 * @param e
 * @param p
 */
void Streamer::process(const Eref& e, ProcPtr p)
{
    // Prepare data.
    zipWithTime( data_, currTime_ );
    StreamerBase::writeToOutFile( outfilePath_, format_, "a", data_, columns_ );

    // clean the arrays
    data_.clear();
}


/**
 * @brief Add a table to streamer.
 *
 * @param table Id of table.
 */
void Streamer::addTable( Id table )
{
    // If this table is not already in the vector, add it.
    for( size_t i = 0; i < tableIds_.size(); i++)
        if( table.path() == tableIds_[i].path() )
            return;                             /* Already added. */

    Table* t = reinterpret_cast<Table*>(table.eref().data());
    tableIds_.push_back( table );
    tables_.push_back( t );
    tableTick_.push_back( table.element()->getTick() );

    // NOTE: If user can make sure that names are unique in table, using name is
    // better than using the full path.
    if( t->getName().size() > 0 )
        columns_.push_back( t->getName( ) );
    else
        columns_.push_back( moose::moosePathToUserPath( table.path() ) );
}

/**
 * @brief Add multiple tables to Streamer.
 *
 * @param tables
 */
void Streamer::addTables( vector<Id> tables )
{
    if( tables.size() == 0 )
        return;
    for( vector<Id>::const_iterator it = tables.begin(); it != tables.end(); it++)
        addTable( *it );
}


/**
 * @brief Remove a table from Streamer.
 *
 * @param table. Id of table.
 */
void Streamer::removeTable( Id table )
{
    int matchIndex = -1;
    for (size_t i = 0; i < tableIds_.size(); i++) 
        if( table.path() == tableIds_[i].path() )
        {
            matchIndex = i;
            break;
        }

    if( matchIndex > -1 )
    {
        tableIds_.erase( tableIds_.begin() + matchIndex );
        tables_.erase( tables_.begin() + matchIndex );
        columns_.erase( columns_.begin() + matchIndex );
    }
}

/**
 * @brief Remove multiple tables -- if found -- from Streamer.
 *
 * @param tables
 */
void Streamer::removeTables( vector<Id> tables )
{
    for( vector<Id>::const_iterator it = tables.begin(); it != tables.end(); it++)
        removeTable( *it );
}

/**
 * @brief Get the number of tables handled by Streamer.
 *
 * @return  Number of tables.
 */
size_t Streamer::getNumTables( void ) const
{
    return tables_.size();
}


string Streamer::getOutFilepath( void ) const
{
    return outfilePath_;
}

void Streamer::setOutFilepath( string filepath )
{
    outfilePath_ = filepath;
    isOutfilePathSet_ = true;
    if( ! moose::createParentDirs( filepath ) )
        outfilePath_ = moose::toFilename( outfilePath_ );

    string format = moose::getExtension( outfilePath_, true );
    if( format.size() > 0)
        setFormat( format );
    else
        setFormat( "csv" );
}

/* Set the format of all Tables */
void Streamer::setFormat( string fmt )
{
    format_ = fmt;
}

/*  Get the format of all tables. */
string Streamer::getFormat( void ) const 
{
    return format_;
}

void Streamer::zipWithTime( vector<double>& data, double currTime)
{
    size_t N = tables_[0]->getVecSize();
    for (size_t i = 0; i < N; i++) 
    {
        /* Each entry we write, currTime_ increases by dt.  */
        data.push_back( currTime_ );
        currTime_ += tableDt_[0];               
        for( size_t i = 0; i < tables_.size(); i++)
            data.push_back( tables_[i]->getVec()[i] );
    }

    // clear the data from tables now.
    for(size_t i = 0; i < tables_.size(); i++ )
        tables_[i]->clearVec();
}

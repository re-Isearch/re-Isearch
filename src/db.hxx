/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */
#ifndef _DB_hxx
#define _DB_hxx

#include <db.h>
#include <fcntl.h>

class DB : public Database
{
    //
    // Construction/Destruction
    //
protected:
    DB();
public:
    ~DB();

    static DB	       *getDatabaseInstance();
	
    virtual int		OpenReadWrite(const STRING& filename, int mode);
    virtual int		OpenRead(const STRING& filename);
    virtual int		Close();
    virtual int		Delete(const STRING& );
	
    virtual void	Start_Get();
    virtual STRING      Get_Next();
    virtual void	Start_Seq(const STRING& str);
    virtual char	*Get_Next_Seq();
	
private:
    STRING              Path;
    int			isOpen;
    DB			*dbp;		// database
    DBC			*dbcp;		// cursor
    DBT			skey;
    DB_ENV		*dbenv;		// database enviroment
    DB_INFO		dbinfo;

    int			seqrc;
    int			seqerr;

    DB_ENV		*db_init(char *);
    virtual int		Get(const STRING&, STRING&);
    virtual int		Put(const STRING&, const STRING&);
    virtual int		Exists(const STRING&);
};

#endif



#include "IBAN.h"
#include "stdafx.h"	

// using

class swift_BIC
{ 

/* 
    4-stelliger Bankcode
    2-stelliger Ländercode
    2-stellige  Codierung des Ortes
    3-stellige  Kennzeichnung der Filiale
*/
 private:
    STRING  bank_id; 
    STRING  LL;
    UINT2   county_code; // Country Code
    STRING  location; // 2nd character:   0 := test  1:= passive 2:= reverse billing 
    STRING  branch_code; // XXX means primary office. 
public: 
    swift_BIC() {
      country_code = 0;
    }
    swift_BIC(string sBIC) { *this = sBIC; }


    swift_BIC& operator  =(const swift_BIC& sBic)
    {
       bank_id      = sBic.bank_id;
       country_code = sBic.country_code;
       LL           = sBic.LL;
       location     = sBin.location;
       branch_code  = sBic.branch_code;

    }

    swift_BIC& operator  =(const STRING& sBic)
    {
        STRING bic = sIBAN;
        bic.removeWhiteSpace(); 
        size_t length = iban.Length(); 
        if (length > 11 || length < 6) return GDT_FALSE; // Need, at least, bank_id and country
        bank_id = bic.substr(0,4).ToUpper(); 
        LL = bic.substr(4,2);
        country_code = iso3166Code2Id( LL );
        if (length < 11) bic.Pad (11-length, 'X'); // We pad with XXXs
        location = bic.substr(6, 2).ToUpper();
        branch_code = bic.substr(8).ToUpper(); 
        return *this;
    }

    GDT_BOOL isTest() const    { return location[1] == '0'; }
    GDT_BOOL isPassive() const { return location[1] == '1'; }
    GDT_BOOL isReverse() const { return location[1] == '2'; }
    GDT_BOOL isPrimaryOffice const { return branch_code[1] == 'X';}


    GDT_BOOL Ok() const { return county_code != 0; }

}


class IBAN
{

    private:
       string iban;
       UINT2  pz;       // Proofsum
       UINT2  ll;       // Country code 
       STRING BLZ_konto;// Bank and account id


    // We store IBAN without spaces
    public: IBAN(string sIBAN) { iban = sIBAN; iban.removeWhiteSpace(); }

    GDT_BOOL Ok() const { return ll != 0; }

    public:

    GDT_BOOL parse() 
    {
        ll = 0;
	// Validate plausible length
        size_t length = iban.Length();
        if (length > 34 || length < 5) return GDT_FALSE;

        STRING country_code = iban.Substring(0, 2);

        ll = iso3166Code2Id( country_code );
        pz = (UINT2)iban.substr(2, 2).getShort(); // Checksum
        BLZ_Konto =   iban.substr(4).ToUpper();

        //Pruefsumme validieren
        LaenderCode = ;
       string Umstellung = BLZ_Konto + country_code + "00";
       string Modulus = IBANCleaner(Umstellung);

       if (98 - Modulo(Modulus, 97) != PZ))
         return false;  //Prüfsumme ist fehlerhaft 

        return true;

    }


    private: string IBANCleaner(string sIBAN)

    {

        for (int x = 65; x <= 90; x++)

        {

            int replacewith = x - 64 + 9;

            string replace = ((char)x).ToString();

            sIBAN = sIBAN.Replace(replace, replacewith.ToString());

        }

        return sIBAN;

    }


    private: int Modulo(string sModulus, int iTeiler)

    {

        int iStart,iEnde,iErgebniss,iRestTmp,iBuffer;

        string iRest = "",sErg = "";


        iStart = 0;

        iEnde = 0;


        while (iEnde <= sModulus.Length - 1)

        {

            iBuffer = int.Parse(iRest + sModulus.Substring(iStart, iEnde - iStart + 1));


            if (iBuffer >= iTeiler)

            {

                iErgebniss = iBuffer / iTeiler;

                iRestTmp = iBuffer - iErgebniss * iTeiler;

                iRest = iRestTmp.ToString();


                sErg = sErg + iErgebniss.ToString();


                iStart = iEnde + 1;

                iEnde = iStart;

            }

            else

            {

                if (sErg != "")

                    sErg = sErg + "0";


                iEnde = iEnde + 1;

            }

        }


        if (iStart <= sModulus.Length)

            iRest = iRest + sModulus.Substring(iStart);


        return int.Parse(iRest);

    }


    private bool IsNumeric(string value)

    {

        try

        {

            int.Parse(value);

            return (true);

        }

        catch

        {

            return (false);

        }

    }


	IBAN_validieren ib = new IBAN_validieren(textBox1.Text);

	if (!ib.ISIBAN())

	{

    MessageBox.Show("IBAN falsch");

	}

}


#pragma once
#include <cmath>
#include <enulib/enu.hpp>
#include <enulib/symbol.hpp>
#include <enulib/asset.hpp>
#include <string>

using namespace enumivo;
using namespace std;

class TokenCreator : public enumivo::contract {
public:
    TokenCreator( account_name self ):contract(self){}

    // @abi action
    [[ enumivo::action ]]
    void transfer( account_name from,
                        account_name to,
                        asset        quantity,
                        string       memo );
    // @abi action
    [[ enumivo::action ]]
    void tokentrans( account_name from,
                        account_name to,
                        asset        quantity,
                        string       memo );
private:
    void _create( account_name issuer,asset maximum_supply);

    void _issue( account_name to, asset quantity, string memo );

    inline asset get_supply( symbol_name sym )const;
         
    inline asset get_balance( account_name owner, symbol_name sym )const;

private:
        //@abi table accounts
        struct [[enumivo::table]] account {
            asset    balance;

            uint64_t primary_key()const { return balance.symbol.name(); }
         };
         typedef enumivo::multi_index<N(accounts), account> accounts;
        
        //@abi table stat
         struct [[enumivo::table]] currency_stats {
            asset          supply;
            asset          max_supply;
            account_name   issuer;

            uint64_t primary_key()const { return supply.symbol.name(); }
         };
         typedef enumivo::multi_index<N(stat), currency_stats> stats;

    void sub_balance( account_name owner, asset value );
    void add_balance( account_name owner, asset value, account_name ram_payer );

    asset get_supply(const string& memo);

    int64_t pow_num(const int& x,const int& y);
};

asset TokenCreator::get_supply( symbol_name sym )const
{
      stats statstable( _self, sym );
      const auto& st = statstable.get( sym );
      return st.supply;
}

asset TokenCreator::get_balance( account_name owner, symbol_name sym )const
{
      accounts accountstable( _self, owner );
      const auto& ac = accountstable.get( sym );
      return ac.balance;
}

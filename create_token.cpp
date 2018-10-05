#include "create_token.hpp"
#include <enulib/symbol.hpp>

extern "C"{
    long long atoll(const char *s)
    {
        long long n=0;
        int neg=0;
        while (isspace(*s)) s++;
        switch (*s) {
        case '-': neg=1;
        case '+': s++;
        }
        while (isdigit(*s))
            n = 10*n - (*s++ - '0');
        return neg ? n : -n;
    }
    int isspace(int c)
    {
        return c == ' ' || (unsigned)c-'\t' < 5;
    }
    int isdigit(int c)
    {
        return (unsigned)c-'0' < 10;
    }
}

int64_t TokenCreator::pow_num(const int& x,const int& y) {
    
    enumivo_assert(y >= 0,"y must great or equal than 0");
    enumivo_assert(x > 0,"x must great than 0");

    int64_t num=1;

    for (int i=0;i<y;i++) {
        num *= x;
    }

    return num;
}

void TokenCreator::sub_balance( account_name owner, asset value ) {
   accounts from_acnts( _self, owner );

   const auto& from = from_acnts.get( value.symbol.name(), "no balance object found" );
   enumivo_assert( from.balance.amount >= value.amount, "overdrawn balance" );


   if( from.balance.amount == value.amount ) {
      from_acnts.erase( from );
   } else {
      from_acnts.modify( from, owner, [&]( auto& a ) {
          a.balance -= value;
      });
   }
}

void TokenCreator::add_balance( account_name owner, asset value, account_name ram_payer )
{
   accounts to_acnts( _self, owner );
   auto to = to_acnts.find( value.symbol.name() );
   if( to == to_acnts.end() ) {
      to_acnts.emplace( ram_payer, [&]( auto& a ){
        a.balance = value;
      });
   } else {
      to_acnts.modify( to, 0, [&]( auto& a ) {
        a.balance += value;
      });
   }
}

asset TokenCreator::get_supply(const string& memo) {

    auto s_token = memo;
    enumivo_assert(s_token.size() > 0,"Memo is empty");

    auto space_pos = s_token.find(' ');
    //if(space_pos == string::npos) {
        //print("space_pos null\n");
    //}
    enumivo_assert(space_pos != string::npos, "Asset's amount and symbol should be separated with space");

    auto s_amount = s_token.substr(0,space_pos);
    auto s_symbol = s_token.substr(space_pos+1);
    //print("amount: ",s_amount,"symbol:",s_symbol,".\n");


    uint8_t precision=0;
    auto dot_pos = s_amount.find('.');
    if (dot_pos != string::npos) {
        enumivo_assert(dot_pos != s_amount.size() - 1, "Missing decimal fraction after decimal point");
        precision=s_amount.size() - dot_pos - 1;
        enumivo_assert(precision <= 18, "Precision should be <= 18");
    } else {
        precision=0;
    }
    //print("precision= ",(int64_t)precision, "\n");

    symbol_type sym = symbol_type(string_to_symbol(precision,s_symbol.c_str()));

    int64_t int_part=0,fract_part=0;
    if (dot_pos != string::npos) {
        int_part = (int64_t)atoll((s_amount.substr(0, dot_pos)).c_str());
        fract_part = (int64_t)atoll((s_amount.substr(dot_pos + 1)).c_str());
    } else {
        int_part = (int64_t)atoll(s_amount.c_str());
    }

    //print("int_part= ", int_part, "  fract_part= ", fract_part, "\n");

    int64_t amount = int_part * pow_num(10,precision) + fract_part;

    //print("total amount= ",amount,"\n");
    
    return asset(amount,sym);
}

void TokenCreator::_create( account_name issuer,
                    asset        maximum_supply )
{
    //require_auth( _self );
    //require_auth( issuer );

    auto sym = maximum_supply.symbol;
    enumivo_assert( sym.is_valid(), "invalid symbol name" );
    enumivo_assert( maximum_supply.is_valid(), "invalid supply");
    enumivo_assert( maximum_supply.amount > 0, "max-supply must be positive");

    stats statstable( _self, sym.name() );
    auto existing = statstable.find( sym.name() );
    enumivo_assert( existing == statstable.end(), "token with symbol already exists 1" );

    statstable.emplace( _self, [&]( auto& s ) {
       s.supply.symbol = maximum_supply.symbol;
       s.max_supply    = maximum_supply;
       s.issuer        = issuer;
    });
}

void TokenCreator::_issue( account_name to, asset quantity, string memo )
{
    //require_auth( _self );

    auto sym = quantity.symbol;
    enumivo_assert( sym.is_valid(), "invalid symbol name" );
    enumivo_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    auto sym_name = sym.name();
    stats statstable( _self, sym_name );
    auto existing = statstable.find( sym_name );
    enumivo_assert( existing != statstable.end(), "token with symbol does not exist, create token before issue" );
    const auto& st = *existing;


    require_auth( st.issuer );
    enumivo_assert( quantity.is_valid(), "invalid quantity" );
    enumivo_assert( quantity.amount > 0, "must issue positive quantity" );

    enumivo_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    enumivo_assert( quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

    statstable.modify( st, 0, [&]( auto& s ) {
       s.supply += quantity;
    });

    //accounts to_acnts( _self, st.issuer );
    //auto to1 = to_acnts.find( quantity.symbol.name() );
    //if (to1 != to_acnts.end()) {
        //const auto& ex = *to1;
        //print("before ",ex.balance.amount,"\n");
    //} else {
        //print("before ",0,"\n");
    //}

    add_balance( st.issuer, quantity, _self );

    //accounts to_acnts2( _self, st.issuer );
    //auto to2 = to_acnts2.find( quantity.symbol.name() );
    //if (to2 != to_acnts2.end()) {
        //const auto& ex = *to2;
        //print("after ",ex.balance.amount,"\n");
    //} else {
        //print("after ",0,"\n");
    //}

    if( to != st.issuer ) {
        //print("to != st.issuer\n");
        SEND_INLINE_ACTION( *this, transfer, {st.issuer,N(active)}, {st.issuer, to, quantity, memo} );
    }
}

void TokenCreator::transfer( account_name from,
                        account_name to,
                        asset        quantity,
                        string       memo ) {
    //print("start transfer\n");
    require_auth( from );

    if (from == _self || (to == _self && memo.size()<=0) ) {
        //print("from == _self or to == _self\n");
        return;
    }

    enumivo_assert(quantity.symbol == S(4, ENU),"must be ENU");
    enumivo_assert(quantity.is_valid(), "invalid token transfer");

    asset max_supply = get_supply(memo);

    //print("max_supply: ",max_supply.amount," ",max_supply.symbol.name(),"\n");

    //create token
    _create(from, max_supply);

    print("create success\n");

    //issue token to from account
    _issue(from,max_supply, ("create "+memo+" success"));

    print("issue success\n");

     //if (quantity.amount > 0) {
      // transfer remain balance to from account
      //INLINE_ACTION_SENDER(enumivo::token, transfer)
      //(N(enu.token), {{_self, N(active)}},
       //{_self, from, asset(quantity),
        //std::string("remain balance")});
    //}

    //print("remain balance back success\n");
}

void TokenCreator::tokentrans( account_name from,
                      account_name to,
                      asset        quantity,
                      string       memo )
{
    //print("tokentrans start\n");

    enumivo_assert( from != to, "cannot transfer to self" );
    require_auth( from );
    enumivo_assert( is_account( to ), "to account does not exist");
    
    enumivo_assert( to != N(enumivo.prods), "enumivo.prods prohibited to receive");

    auto sym = quantity.symbol.name();
    stats statstable( _self, sym );
    const auto& st = statstable.get( sym );

    require_recipient( from );
    require_recipient( to );

    enumivo_assert( quantity.is_valid(), "invalid quantity" );
    enumivo_assert( quantity.amount > 0, "must transfer positive quantity" );
    enumivo_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    enumivo_assert( memo.size() <= 256, "memo has more than 256 bytes" );


    sub_balance( from, quantity );
    add_balance( to, quantity, from );

    //print("tokentrans success\n");
}
//ENUMIVO_ABI(TokenCreator, (transfer)(tokentrans)(getbalance))

#define ENUMIVO_ABI_EX( TYPE, MEMBERS ) \
extern "C" { \
   void apply( uint64_t receiver, uint64_t code, uint64_t action ) { \
      auto self = receiver; \
      /*print("enu.token:",N(enu.token)," tokencreator:",N(tokencreator)," code:",code,"\n");*/ \
      if( action == N(onerror)) { \
         /* onerror is only valid if it is for the "enumivo" code account and authorized by "enumivo"'s "active permission */ \
         enumivo_assert(code == N(enumivo), "onerror action's are only valid from the \"enumivo\" system account"); \
      } \
      uint64_t _action = N(onerror); \
      if(code == N(enu.token) && action == N(transfer)) { \
          /*print("enu.token transfer\n");*/ \
          _action = N(transfer); \
      } else if(code == N(tokencreator)) { \
          /*print("tokencreator transfer\n");*/ \
          if(action == N(transfer)) { \
              _action = N(tokentrans); \
          } \
      } \
      if(_action == N(onerror)) { \
          enumivo_assert(false, "action from this code is denied"); \
      } \
      TYPE thiscontract( self ); \
      switch( _action ) { \
          ENUMIVO_API( TYPE, MEMBERS ) \
      } \
   } \
} \

ENUMIVO_ABI_EX(TokenCreator, (transfer)(tokentrans))
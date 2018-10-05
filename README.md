# tokencreator
A smart contract for create tokens on enumivo
一个在enumivo上用于创建token的智能合约

Send 0.0001 ENU to account **tokencreator** and write memo with your token amount and asset symbol(like: ***100000000.0000 TC***), and then your account will recive the token you issue.

向账户 **tokencreator** 发送0.0001 ENU，在memo里填写你的token的发行数量和符号(例如：***100000000.0000 TC***)，然后你的账号就会收到你发行的token.

## command line 命令行:

### create token 创建
```
./enucli -u https://api.enumivo.com/ push action enu.token transfer '["youraccount","tokencreator","0.0001 ENU","1000000000.0000 TC"]' -p youraccount@active
```

### transfer token 转账
```
./enucli -u https://api.enumivo.com/ push action tokencreator transfer '["youraccount","toaccount","1.0000 TC","trans memo"]' -p youraccount@active
```

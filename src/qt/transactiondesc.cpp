#include <string>

#include "guiutil.h"

#include "base58.h"
#include "db.h"
#include "wallet.h"
#include "ui_interface.h"

#include "coinunits.h"
#include "transactionrecord.h"
#include "transactiondesc.h"

QString TransactionDesc::FormatTxStatus(const CWalletTx& wtx)
{
    if (!wtx.IsFinal())
    {
        if (wtx.nLockTime < LOCKTIME_THRESHOLD)
            return tr("Open for %n block(s)", "", nBestHeight - wtx.nLockTime);
        else
            return tr("Open until %1").arg(GUIUtil::dateTimeStr(wtx.nLockTime));
    }
    else
    {
        int nDepth = wtx.GetDepthInMainChain();
        if (GetAdjustedTime() - wtx.nTimeReceived > 2 * 60 && wtx.GetRequestCount() == 0)
            return tr("%1/offline").arg(nDepth);
        else if (nDepth < 6)
            return tr("%1/unconfirmed").arg(nDepth);
        else
            return tr("%1 confirmations").arg(nDepth);
    }
}

QString TransactionDesc::toHTML(CWallet *wallet, CWalletTx &wtx, int unit) {
    QString strHTML;

    {
        LOCK(wallet->cs_wallet);
        strHTML.reserve(4000);
        strHTML += "<html><font face='verdana, arial, helvetica, sans-serif'>";

        int64 nTime = wtx.GetTxTime();
        int64 nCredit = wtx.GetCredit();
        int64 nDebit = wtx.GetDebit();
        int64 nNet = nCredit - nDebit;

        strHTML += "<b>" + tr("Status") + ":</b> " + FormatTxStatus(wtx);
        int nRequests = wtx.GetRequestCount();
        if (nRequests != -1)
        {
            if (nRequests == 0)
                strHTML += tr(", has not been successfully broadcast yet");
            else if (nRequests > 0)
                strHTML += tr(", broadcast through %n node(s)", "", nRequests);
        }
        strHTML += "<br>";

        strHTML += "<b>" + tr("Date") + ":</b> " + (nTime ? GUIUtil::dateTimeStr(nTime) : "") + "<br>";

        //
        // From
        //
        if(wtx.IsCoinBase()) {
            strHTML += "<b>" + tr("Source") + ":</b> " + tr("generated") + "<br>";
        }
        else if (wtx.mapValue.count("from") && !wtx.mapValue["from"].empty())
        {
            // Online transaction
            strHTML += "<b>" + tr("From") + ":</b> " + GUIUtil::HtmlEscape(wtx.mapValue["from"]) + "<br>";
        }
        else
        {
            // Offline transaction
            if (nNet > 0)
            {
                // Credit
                BOOST_FOREACH(const CTxOut& txout, wtx.vout)
                {
                    if (wallet->IsMine(txout))
                    {
                        CTxDestination address;
                        if (ExtractDestination(txout.scriptPubKey, address) && IsMine(*wallet, address))
                        {
                            if (wallet->mapAddressBook.count(address))
                            {
                                strHTML += "<b>" + tr("From") + ":</b> " + tr("unknown") + "<br>";
                                strHTML += "<b>" + tr("To") + ":</b> ";
                                strHTML += GUIUtil::HtmlEscape(CCoinAddress(address).ToString());
                                if (!wallet->mapAddressBook[address].empty())
                                    strHTML += " (" + tr("own address") + ", " + tr("label") + ": " + GUIUtil::HtmlEscape(wallet->mapAddressBook[address]) + ")";
                                else
                                    strHTML += " (" + tr("own address") + ")";
                                strHTML += "<br>";
                            }
                        }
                        break;
                    }
                }
            }
        }

        //
        // To
        //
        if (wtx.mapValue.count("to") && !wtx.mapValue["to"].empty())
        {
            // Online transaction
            std::string strAddress = wtx.mapValue["to"];
            strHTML += "<b>" + tr("To") + ":</b> ";
            CTxDestination dest = CCoinAddress(strAddress).Get();
            if (wallet->mapAddressBook.count(dest) && !wallet->mapAddressBook[dest].empty())
                strHTML += GUIUtil::HtmlEscape(wallet->mapAddressBook[dest]) + " ";
            strHTML += GUIUtil::HtmlEscape(strAddress) + "<br>";
        }

        //
        // Amount
        //
        if (wtx.IsCoinBase() && nCredit == 0)
        {
            //
            // Coinbase
            //
            int64 nUnmatured = 0;
            BOOST_FOREACH(const CTxOut& txout, wtx.vout)
                nUnmatured += wallet->GetCredit(txout);
            strHTML += "<b>" + tr("Credit") + ":</b> ";
            if(wtx.IsInMainChain()) {
                strHTML += CoinUnits::formatWithUnit(unit, nUnmatured) +
                  " (" + tr("matures in %n more block(s)", "", wtx.GetBlocksToMaturity()) + ")";
            } else {
                strHTML += "(" + tr("not accepted") + ")";
            }
            strHTML += "<br>";
        }
        else if (nNet > 0)
        {
            //
            // Credit
            //
            strHTML += "<b>" + tr("Credit") + ":</b> " + CoinUnits::formatWithUnit(unit, nNet) + "<br>";
        }
        else
        {
            bool fAllFromMe = true;
            BOOST_FOREACH(const CTxIn& txin, wtx.vin)
                fAllFromMe = fAllFromMe && wallet->IsMine(txin);

            bool fAllToMe = true;
            BOOST_FOREACH(const CTxOut& txout, wtx.vout)
                fAllToMe = fAllToMe && wallet->IsMine(txout);

            if (fAllFromMe)
            {
                //
                // Debit
                //
                BOOST_FOREACH(const CTxOut& txout, wtx.vout)
                {
                    if (wallet->IsMine(txout))
                        continue;

                    if (!wtx.mapValue.count("to") || wtx.mapValue["to"].empty())
                    {
                        // Offline transaction
                        CTxDestination address;
                        if (ExtractDestination(txout.scriptPubKey, address))
                        {
                            strHTML += "<b>" + tr("To") + ":</b> ";
                            if (wallet->mapAddressBook.count(address) && !wallet->mapAddressBook[address].empty())
                                strHTML += GUIUtil::HtmlEscape(wallet->mapAddressBook[address]) + " ";
                            strHTML += GUIUtil::HtmlEscape(CCoinAddress(address).ToString());
                            strHTML += "<br>";
                        }
                    }

                    strHTML += "<b>" + tr("Debit") + ":</b> " +
                      CoinUnits::formatWithUnit(unit, -txout.nValue) + "<br>";
                }

                if (fAllToMe)
                {
                    // Payment to self
                    int64 nChange = wtx.GetChange();
                    int64 nValue = nCredit - nChange;
                    strHTML += "<b>" + tr("Debit") + ":</b> " +
                      CoinUnits::formatWithUnit(unit, -nValue) + "<br>";
                    strHTML += "<b>" + tr("Credit") + ":</b> " +
                      CoinUnits::formatWithUnit(unit, nValue) + "<br>";
                }

                int64 nTxFee = nDebit - wtx.GetValueOut();
                if(nTxFee > 0) strHTML += "<b>" + tr("Transaction fee") + ":</b> " +
                  CoinUnits::formatWithUnit(unit, -nTxFee) + "<br>";
            }
            else
            {
                //
                // Mixed debit transaction
                //
                BOOST_FOREACH(const CTxIn &txin, wtx.vin)
                  if(wallet->IsMine(txin)) strHTML += "<b>" + tr("Debit") + ":</b> " +
                    CoinUnits::formatWithUnit(unit, -wallet->GetDebit(txin)) + "<br>";

                BOOST_FOREACH(const CTxOut &txout, wtx.vout)
                  if(wallet->IsMine(txout)) strHTML += "<b>" + tr("Credit") + ":</b> " +
                    CoinUnits::formatWithUnit(unit, wallet->GetCredit(txout)) + "<br>";
            }
        }

        strHTML += "<b>" + tr("Net amount") + ":</b> " + CoinUnits::formatWithUnit(unit, nNet, true) + "<br>";

        //
        // Message
        //
        if (wtx.mapValue.count("message") && !wtx.mapValue["message"].empty())
            strHTML += "<br><b>" + tr("Message") + ":</b><br>" + GUIUtil::HtmlEscape(wtx.mapValue["message"], true) + "<br>";
        if (wtx.mapValue.count("comment") && !wtx.mapValue["comment"].empty())
            strHTML += "<br><b>" + tr("Comment") + ":</b><br>" + GUIUtil::HtmlEscape(wtx.mapValue["comment"], true) + "<br>";

        strHTML += "<b>" + tr("Transaction ID") + ":</b> " + wtx.GetHash().ToString().c_str() + "<br>";

        if(wtx.IsCoinBase()) {
            strHTML += "<br>" + tr("Generated coins must mature before they can be spent. When you generated this block, it was broadcast to the network to be added to the block chain. If it fails to get into the chain, its state will change to \"not accepted\" and it won't be spendable. This may occasionally happen if another node generates a block within a few seconds of yours.") + "<br>";
        }

        /* Extended transaction information */

        uint i;

        strHTML += "<hr><br><b>" + tr("Inputs") + ":</b>";
        strHTML += "<ul>";

        for(i = 0; i < wtx.vin.size(); i++) {
            uint256 hash_in, hashBlock_in = 0;
            const CTxIn &vin = wtx.vin[i];

            hash_in = vin.prevout.hash;
            CTransaction txPrev;
            if(!GetTransaction(hash_in, txPrev, hashBlock_in)) {
                /* Must be a coin base */
                strHTML += "<li>" + tr("(generated)") + "</li>";
                continue;
            }

            strHTML += "<li>";

            CTxDestination address;
            if(ExtractDestination(txPrev.vout[vin.prevout.n].scriptPubKey, address)) {
                if(wallet->mapAddressBook.count(address) &&
                  !wallet->mapAddressBook[address].empty()) {
                    strHTML += GUIUtil::HtmlEscape(wallet->mapAddressBook[address]);
                    /* Qt replaces &ensp; or &emsp; with a single white space */
                    strHTML += "&nbsp;&nbsp;";
                }
                strHTML += QString::fromStdString(CCoinAddress(address).ToString());
            }

            strHTML += "&nbsp;&nbsp;" + CoinUnits::formatWithUnit(unit,
              txPrev.vout[vin.prevout.n].nValue);

            if(wallet->IsMine(txPrev.vout[vin.prevout.n]) & MINE_SPENDABLE)
              strHTML += "&nbsp;&nbsp;" + tr("(mine)");

            if(wallet->IsMine(txPrev.vout[vin.prevout.n]) & MINE_WATCH_ONLY)
              strHTML += "&nbsp;&nbsp;" + tr("(watch)");

            strHTML += "</li>";
        }

        strHTML += "</ul>";

        strHTML += "<hr><br><b>" + tr("Outputs") + ":</b>";
        strHTML += "<ul>";

        for(i = 0; i < wtx.vout.size(); i++) {
            const CTxOut &txout = wtx.vout[i];

            strHTML += "<li>";

            CTxDestination address;
            if(ExtractDestination(txout.scriptPubKey, address)) {
                if(wallet->mapAddressBook.count(address) &&
                  !wallet->mapAddressBook[address].empty()) {
                    strHTML += GUIUtil::HtmlEscape(wallet->mapAddressBook[address]);
                    strHTML += "&nbsp;&nbsp;";
                }
                strHTML += QString::fromStdString(CCoinAddress(address).ToString());
            }

            strHTML += "&nbsp;&nbsp;" + CoinUnits::formatWithUnit(unit, txout.nValue);

            if(wallet->IsMine(txout) & MINE_SPENDABLE)
              strHTML += "&nbsp;&nbsp;" + tr("(mine)");

            if(wallet->IsMine(txout) & MINE_WATCH_ONLY)
              strHTML += "&nbsp;&nbsp;" + tr("(watch)");

            strHTML += "</li>";
        }

        strHTML += "</ul>";

        if(fDebug) {
            strHTML += "<hr><br><b>" + tr("Transaction dump") + ":</b><br><br>";
            strHTML += GUIUtil::HtmlEscape(wtx.ToString(), true);
        } else {
            strHTML += "<br>";
        }

        strHTML += "</font></html>";
    }

    return(strHTML);
}

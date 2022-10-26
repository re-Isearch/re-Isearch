## This is the respositiory for re-Isearch.

### Project re-isearch:
a novel multimodal search and retrieval engine using mathematical models and algorithms different from the all-too-common inverted index (popularized by Salton in the 1960s). The design allows it to have, in practice, effectively no limits on the frequency of words, term length, number of fields or complexity of structured data and support even overlap--- where fields or structures cross other's boundaries (common examples are quotes, line/sentences, biblical verse, annotations). Its model enables a completely flexible unit of retrieval and modes of search.

Featues/Uses

    • Low-code ETL / "Any-to-Any" architecture
   • No need for a “middle layer” of content manipulation code. Instead of getting URLs from a search engine, fetching documents, parsing them, and navigating the DOMs to find required elements, it lets you simply request the elements you need and they are returned directly.
    • Handles a wide range of document formats (from Atom to XML) including “live” data.
    • Powerful Search (Structure, Objects, Spatial) / Relevancy Engine
    • NoSQL Datastore
    • Useful for Analytics
    • Useful for Recommendation / Autosuggestion 
    • Embeddable in products (comparatively low resource  demands)
    • Customization. 
    • Support Peer-to-Peer and Federated architectures.
    • Runs on a wide range of hardware and operating systems
    • Freely available under a permissive software license. 


Despite its wealth of features it has a comparatively small memory footprint (previous version have run on 32-bit machines with as little as 8 MB physical RAM) making it suitable for appliances. It has also been designed to try to impose a minimal computing impact on the host.
Rather than run multiple threads and a high CPU workload it’s strategy is to be fast but not at the cost of other processes, heat or increased energy consumption.


### This Repository 

This is the main central repository for re-Isearch development.

It contains the engine as a freely available and completely open-source (and multiplatform) C++ library, bindings for other languages (such as Python) and some reference sample code using the library in some of these languages.

Under [doctypes/](https://github.com/re-Isearch/re-Isearch/tree/master/doctype) one can see the native doctypes supported.


## Building, installing, developing
For information on building, installing, developing and using the system please consult the handbook in [docs/](https://github.com/re-Isearch/re-Isearch/blob/master/docs/re-Isearch-Handbook.pdf).

A basic cheat-sheet is in [INSTALLATION](./INSTALLATION)

In the directory bin/ and lib/ are binaries of standalone tools compiled on Ubuntu 18.04.2 LTS and targetting Intel Skylake or newer processors. They are included solely to enable fast software evaluations.

## Copyrights, attributions and acknowledgements 
Portions Copyright (c) 1995 CNIDR/MCNC, (c) 1995-2011 BSn/Munich; (c) 2011-2020 NONMONOTONIC Networks; Copyright (c) 2020-22 Edward C. Zimmermann and the re-iSearch project. Is is made available and licensed under the Apache 2.0 license: see LICENSE

The software has a lot of history (as one can see from the above copyright). For the historical last public release: [Isearch](https://github.com/edzimmermann/Isearch-1.14)

This project was funded through the NGI0 Discovery Fund, a fund established by NLnet with financial support from the European Commission's Next Generation Internet programme, under the aegis of DG Communications Networks, Content and Technology under grant agreement No 825322.



<IMG SRC="https://nlnet.nl/image/logo_nlnet.svg" ALT="NLnet Foundation" height=100> <IMG SRC="https://nlnet.nl/logo/NGI/NGIZero-green.hex.svg" ALT="NGI0 Search" height=100> &nbsp; &nbsp; <IMG SRC="https://ngi.eu/wp-content/uploads/sites/77/2017/10/bandiera_stelle.png" ALT="EU" height=100>

